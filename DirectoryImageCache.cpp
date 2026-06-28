#include "DirectoryImageCache.h"

#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QImageReader>
#include <QStandardPaths>
#include <QByteArray>

#include <iostream>
#include <fstream>
#include <vector>

#include "stopwatch.h"

QByteArray extractThumbnailRawToBuffer(const std::string& imagePath) {
    Stopwatch timer(0, "extractThumbnailRawToBuffer");

    std::ifstream file(imagePath, std::ios::binary);
    if (!file) return QByteArray();

    // Gesamte Datei in einen Vektor laden
    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    size_t firstSOI = std::string::npos;
    size_t secondSOI = std::string::npos;
    size_t thumbnailEOI = std::string::npos;

    // Suche nach JPEG-Markern (0xFF, 0xD8) und (0xFF, 0xD9)
    for (size_t i = 0; i < buffer.size() - 1; ++i) {
        if (buffer[i] == 0xFF && buffer[i+1] == 0xD8) {
            if (firstSOI == std::string::npos) {
                firstSOI = i;
            } else if (secondSOI == std::string::npos) {
                secondSOI = i; // Thumbnail Anfang
            }
        }
        if (secondSOI != std::string::npos && buffer[i] == 0xFF && buffer[i+1] == 0xD9) {
            thumbnailEOI = i + 2; // Thumbnail Ende
            break;
        }
    }

    // Wenn ein valides Thumbnail-Segment gefunden wurde
    if (secondSOI != std::string::npos && thumbnailEOI != std::string::npos) {
        size_t thumbnailLength = thumbnailEOI - secondSOI;

        // Daten direkt als QByteArray zurückgeben (kopiert die Rohdaten in Qt-Speicher)
        return QByteArray(reinterpret_cast<const char*>(&buffer[secondSOI]), static_cast<int>(thumbnailLength));
    }

    return QByteArray(); // Leer, falls kein Thumbnail gefunden wurde
}

QByteArray extractThumbnailFast(const std::string& imagePath) {
    Stopwatch timer(0, "extractThumbnailFast");

    std::ifstream file(imagePath, std::ios::binary);
    if (!file) return QByteArray();

    // EXIF APP1-Segmente sind laut JPEG-Spezifikation auf maximal 64 KB begrenzt.
    // Wir lesen daher NUR die ersten 65.536 Bytes ein. Das spart 99% des I/O-Overheads!
    const size_t maxHeaderSize = 65536;
    std::vector<uint8_t> headerBuffer(maxHeaderSize);

    file.read(reinterpret_cast<char*>(headerBuffer.data()), maxHeaderSize);
    std::streamsize bytesRead = file.gcount();

    if (bytesRead < 4) return QByteArray();

    size_t firstSOI = std::string::npos;
    size_t secondSOI = std::string::npos;
    size_t thumbnailEOI = std::string::npos;

    // Wir durchsuchen nur den winzigen Header im RAM
    for (size_t i = 0; i < static_cast<size_t>(bytesRead) - 1; ++i) {
        if (headerBuffer[i] == 0xFF && headerBuffer[i+1] == 0xD8) {
            if (firstSOI == std::string::npos) {
                firstSOI = i; // Hauptbild-Anfang (ganz oben in der Datei)
            } else {
                secondSOI = i; // Thumbnail-Anfang im EXIF-Header gefunden!
                break;         // Schleife sofort abbrechen!
            }
        }
    }

    // Wenn der Thumbnail-Anfang gefunden wurde, suchen wir das Ende (EOI: 0xFFD9)
    if (secondSOI != std::string::npos) {
        for (size_t i = secondSOI; i < static_cast<size_t>(bytesRead) - 1; ++i) {
            if (headerBuffer[i] == 0xFF && headerBuffer[i+1] == 0xD9) {
                thumbnailEOI = i + 2; // Thumbnail-Ende gefunden
                break;
            }
        }
    }

    // Falls gefunden, direkt den extrahierten Bereich in das QByteArray kopieren
    if (secondSOI != std::string::npos && thumbnailEOI != std::string::npos) {
        size_t thumbLength = thumbnailEOI - secondSOI;
        return QByteArray(reinterpret_cast<const char*>(&headerBuffer[secondSOI]), static_cast<int>(thumbLength));
    }

    return QByteArray(); // Kein eingebettetes JPEG-Thumbnail im Header vorhanden
}

QImage extractThumbnail(const std::string& imagePath) {
    QByteArray data = extractThumbnailFast(imagePath);
    QImage thumbnailImage;
    if (thumbnailImage.loadFromData(data)) {
        return thumbnailImage;
    }
    return QImage();
}

const QStringList DirectoryImageCache::s_supportedExtensions = {
    "jpg", "jpeg", "png", "gif", "bmp", "webp", "svg", "tiff", "tif"
};

std::unique_ptr<DirectoryImageCache> DirectoryImageCache::s_instance = nullptr;

DirectoryImageCache *DirectoryImageCache::instance()
{
    if (!s_instance) {
        s_instance = std::make_unique<DirectoryImageCache>();
    }
    return s_instance.get();
}

DirectoryImageCache::DirectoryImageCache(const QString &directoryPath, QObject *parent)
    : QObject(parent)
    , m_directoryPath(directoryPath)
{
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged,
            this,       &DirectoryImageCache::onDirectoryChanged);
    updateWatcher();
    generateThumbnails();
}

void DirectoryImageCache::setDirectoryPath(const QString &directoryPath)
{
    if (m_directoryPath == directoryPath)
        return;

    m_directoryPath = directoryPath;
    updateWatcher();
    clear();
    generateThumbnails();
}

QString DirectoryImageCache::directoryPath() const
{
    return m_directoryPath;
}

void DirectoryImageCache::generateThumbnails()
{
    clear();
    QList<QFileInfo> fileInfos;
    buildFileList(fileInfos);

    for (const QFileInfo &info : fileInfos) {
        const QString fileName = info.fileName();
        const QString filePath = info.absoluteFilePath();
        CacheItem item;
        item.filePath = filePath;
        item.lastModified = info.lastModified();
        item.thumbnail = extractThumbnail(filePath.toStdString().c_str()); //createThumbnail(filePath);
        if (!item.thumbnail.isNull()) {
            m_cache.insert(fileName, std::move(item));
        }
    }
}

bool DirectoryImageCache::update()
{
    QList<QFileInfo> fileInfos;
    buildFileList(fileInfos);

    QHash<QString, CacheItem> newCache;
    bool changed = false;

    for (const QFileInfo &info : fileInfos) {
        const QString fileName = info.fileName();
        const QString filePath = info.absoluteFilePath();
        const QDateTime lastModified = info.lastModified();

        const auto it = m_cache.find(fileName);
        if (it != m_cache.end() && it->filePath == filePath && it->lastModified == lastModified) {
            newCache.insert(fileName, *it);
        } else {
            CacheItem item;
            item.filePath = filePath;
            item.lastModified = lastModified;
            item.thumbnail = extractThumbnail(filePath.toStdString().c_str()); //createThumbnail(filePath);
            newCache.insert(fileName, std::move(item));
            changed = true;
        }
    }

    if (newCache.size() != m_cache.size())
        changed = true;

    m_cache = std::move(newCache);
    return changed;
}

bool DirectoryImageCache::save(const QString &cacheFilePath) const
{
    QFile file(cacheFilePath);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_6_0);
    out << QStringLiteral("DirectoryImageCache")
        << quint32(1)
        << m_directoryPath
        << quint32(m_cache.size());

    for (auto it = m_cache.constBegin(); it != m_cache.constEnd(); ++it) {
        out << it.key();
        out << it->filePath;
        out << it->lastModified;
        out << it->thumbnail;
    }

    return file.error() == QFileDevice::NoError;
}

bool DirectoryImageCache::load(const QString &cacheFilePath)
{
    QFile file(cacheFilePath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_6_0);

    QString header;
    quint32 version;
    in >> header >> version;
    if (header != QLatin1String("DirectoryImageCache") || version != 1)
        return false;

    QString loadedPath;
    in >> loadedPath;
    m_directoryPath = std::move(loadedPath);
    updateWatcher();

    quint32 itemCount;
    in >> itemCount;
    m_cache.clear();
    for (quint32 i = 0; i < itemCount; ++i) {
        QString fileName;
        CacheItem item;
        in >> fileName;
        in >> item.filePath;
        in >> item.lastModified;
        in >> item.thumbnail;
        if (!fileName.isEmpty() && !item.thumbnail.isNull())
            m_cache.insert(fileName, std::move(item));
    }

    return file.error() == QFileDevice::NoError;
}

void DirectoryImageCache::updateWatcher()
{
    const QString normalizedPath = QDir::cleanPath(m_directoryPath);

    if (m_watcher.directories().contains(normalizedPath))
        return;

    if (!m_watcher.directories().isEmpty())
        m_watcher.removePaths(m_watcher.directories());

    if (!normalizedPath.isEmpty() && QDir(normalizedPath).exists()) {
        m_watcher.addPath(normalizedPath);
    }
}

void DirectoryImageCache::onDirectoryChanged(const QString &path)
{
    Q_UNUSED(path)
    update();
}

QImage DirectoryImageCache::thumbnailForFileName(const QString &fileName) const
{
    const auto it = m_cache.constFind(fileName);
    return it != m_cache.constEnd() ? it->thumbnail : QImage();
}

bool DirectoryImageCache::hasThumbnail(const QString &fileName) const
{
    return m_cache.contains(fileName);
}

QStringList DirectoryImageCache::imageFileNames() const
{
    return m_cache.keys();
}

void DirectoryImageCache::clear()
{
    m_cache.clear();
}

void DirectoryImageCache::buildFileList(QList<QFileInfo> &fileInfos) const
{
    fileInfos.clear();
    if (m_directoryPath.isEmpty())
        return;

    QDir dir(m_directoryPath);
    if (!dir.exists())
        return;

    const QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::NoSymLinks, QDir::Name);
    for (const QFileInfo &info : entries) {
        if (isImageFile(info))
            fileInfos.append(info);
    }
}

QImage DirectoryImageCache::createThumbnail(const QString &filePath) const
{
    Stopwatch timer(0, "createThumbnail");

    QImageReader reader(filePath);
    reader.setAutoTransform(true);
    QImage image = reader.read();
    if (image.isNull())
        return QImage();

    const QSize maxSize(160, 160);
    if (image.width() > image.height()) {
        return image.scaledToWidth(maxSize.width(), Qt::SmoothTransformation);
    }
    return image.scaledToHeight(maxSize.height(), Qt::SmoothTransformation);
}

bool DirectoryImageCache::isImageFile(const QFileInfo &fileInfo)
{
    const QString suffix = fileInfo.suffix().toLower();
    return s_supportedExtensions.contains(suffix);
}
