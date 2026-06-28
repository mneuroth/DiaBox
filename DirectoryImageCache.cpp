#include "DirectoryImageCache.h"

#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QImageReader>
#include <QStandardPaths>

const QStringList DirectoryImageCache::s_supportedExtensions = {
    "jpg", "jpeg", "png", "gif", "bmp", "webp", "svg", "tiff", "tif"
};

DirectoryImageCache::DirectoryImageCache(const QString &directoryPath)
    : m_directoryPath(directoryPath)
{
    generateThumbnails()
}

void DirectoryImageCache::setDirectoryPath(const QString &directoryPath)
{
    if (m_directoryPath == directoryPath)
        return;

    m_directoryPath = directoryPath;
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
        item.thumbnail = createThumbnail(filePath);
        if (!item.thumbnail.isNull())
            m_cache.insert(fileName, std::move(item));
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
            item.thumbnail = createThumbnail(filePath);
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
