#include "DirectoryModel.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QStandardPaths>
#include <QUrl>

// Supported image extensions (lower-case; upper-case variants are added in refresh())
const QStringList DirectoryModel::s_extensions = {
    "jpg", "jpeg", "png", "gif", "bmp", "webp", "svg", "tiff", "tif"
};

// ── Construction ─────────────────────────────────────────────────────────────

DirectoryModel::DirectoryModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged,
            this,       &DirectoryModel::onDirectoryChanged);
}

// ── Q_PROPERTY: folder ────────────────────────────────────────────────────────

QString DirectoryModel::folder() const
{
    return m_folder;
}

void DirectoryModel::setFolder(const QString &folder)
{
    if (m_folder == folder)
        return;

    // Stop watching the old path
    if (!m_folder.isEmpty() && m_watcher.directories().contains(m_folder))
        m_watcher.removePath(m_folder);

    m_folder = folder;

    // Start watching the new path (only if it exists)
    if (QDir(m_folder).exists())
        m_watcher.addPath(m_folder);

    refresh();
    emit folderChanged();
}

// ── QAbstractListModel interface ──────────────────────────────────────────────

int DirectoryModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_files.size();
}

QVariant DirectoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_files.size())
        return {};

    const QString &name = m_files.at(index.row());

    const QString fullPath = QDir(m_folder).absoluteFilePath(name);

    switch (role) {
    case FileNameRole:
    case Qt::DisplayRole:
        return name;
    case FilePathRole:
        return fullPath;
    case FileUrlRole:
        return QUrl::fromLocalFile(fullPath);
    case ThumbnailUrlRole:
        return thumbnailUrlForFile(fullPath);
    }
    return {};
}

QHash<int, QByteArray> DirectoryModel::roleNames() const
{
    return {
        { FileNameRole,      "fileName"      },
        { FilePathRole,      "filePath"      },
        { FileUrlRole,       "fileUrl"       },
        { ThumbnailUrlRole,  "thumbnailUrl"  },
    };
}

// ── Q_INVOKABLE helpers ───────────────────────────────────────────────────────

QUrl DirectoryModel::fileUrl(int row) const
{
    if (row < 0 || row >= m_files.size())
        return {};
    return QUrl::fromLocalFile(QDir(m_folder).absoluteFilePath(m_files.at(row)));
}

QUrl DirectoryModel::thumbnailUrl(int row) const
{
    if (row < 0 || row >= m_files.size())
        return {};
    return thumbnailUrlForFile(QDir(m_folder).absoluteFilePath(m_files.at(row)));
}

QString DirectoryModel::fileName(int row) const
{
    if (row < 0 || row >= m_files.size())
        return {};
    return m_files.at(row);
}

// ── Private slots / helpers ───────────────────────────────────────────────────

void DirectoryModel::onDirectoryChanged(const QString &path)
{
    Q_UNUSED(path)
    refresh();
}

QUrl DirectoryModel::thumbnailUrlForFile(const QString &filePath) const
{
    const auto cacheIt = m_thumbnailCache.constFind(filePath);
    if (cacheIt != m_thumbnailCache.constEnd())
        return *cacheIt;

    const QFileInfo sourceInfo(filePath);
    if (!sourceInfo.exists()) {
        const QUrl fallback = QUrl::fromLocalFile(filePath);
        m_thumbnailCache.insert(filePath, fallback);
        return fallback;
    }

    const QString cachePath = thumbnailCachePath(filePath);
    if (!cachePath.isEmpty() && QFileInfo::exists(cachePath)) {
        const QUrl cachedUrl = QUrl::fromLocalFile(cachePath);
        m_thumbnailCache.insert(filePath, cachedUrl);
        return cachedUrl;
    }

    QImageReader reader(filePath);
    reader.setAutoTransform(true);

    QImage thumbnail = reader.read();
    if (thumbnail.isNull()) {
        const QUrl fallback = QUrl::fromLocalFile(filePath);
        m_thumbnailCache.insert(filePath, fallback);
        return fallback;
    }

    const QSize maxSize(160, 160);
    if (thumbnail.width() > thumbnail.height()) {
        thumbnail = thumbnail.scaledToWidth(maxSize.width(), Qt::SmoothTransformation);
    } else {
        thumbnail = thumbnail.scaledToHeight(maxSize.height(), Qt::SmoothTransformation);
    }
    if (thumbnail.isNull()) {
        const QUrl fallback = QUrl::fromLocalFile(filePath);
        m_thumbnailCache.insert(filePath, fallback);
        return fallback;
    }

    const QFileInfo cacheInfo(cachePath);
    if (!cacheInfo.dir().exists()) {
        QDir().mkpath(cacheInfo.absolutePath());
    }

    if (thumbnail.save(cachePath, "PNG")) {
        const QUrl cachedUrl = QUrl::fromLocalFile(cachePath);
        m_thumbnailCache.insert(filePath, cachedUrl);
        return cachedUrl;
    }

    const QUrl fallback = QUrl::fromLocalFile(filePath);
    m_thumbnailCache.insert(filePath, fallback);
    return fallback;
}

QString DirectoryModel::thumbnailCachePath(const QString &filePath) const
{
    const QByteArray hash = QCryptographicHash::hash(filePath.toUtf8(), QCryptographicHash::Sha256).toHex();
    const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                             + QStringLiteral("/DiaBox/thumbnails");
    qDebug() << "CACHE: " << cacheDir << Qt::endl;  // C:/Users/<user>/AppData/Local/mneuroth/DiaBox/cache/DiaBox/thumbnails
    return QDir(cacheDir).absoluteFilePath(QStringLiteral("%1.png").arg(QString::fromUtf8(hash)));
}

void DirectoryModel::clearThumbnailCache()
{
    m_thumbnailCache.clear();
}

void DirectoryModel::refresh()
{
    QDir dir(m_folder);

    QStringList files;
    if (dir.exists()) {
        // Build name filters for both lower- and upper-case extensions
        QStringList nameFilters;
        for (const QString &ext : s_extensions) {
            nameFilters << QStringLiteral("*.") + ext
                        << QStringLiteral("*.") + ext.toUpper();
        }
        files = dir.entryList(nameFilters, QDir::Files, QDir::Name);
    }

    beginResetModel();
    clearThumbnailCache();
    m_files = files;
    endResetModel();

    emit countChanged();
}
