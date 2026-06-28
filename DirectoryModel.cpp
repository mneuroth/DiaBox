#include "DirectoryModel.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QStandardPaths>
#include <QUrl>

#include "DirectoryImageCache.h"

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
    DirectoryImageCache::instance()->setDirectoryPath(m_folder);

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
    case ThumbnailImgRole: {
        // Return image://cache/ URL for the cached thumbnail
        return QUrl(QString("image://cache/%1").arg(name));
    }
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
        { ThumbnailImgRole,  "thumbnailImg"  },
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

QImage DirectoryModel::thumbnailForFile(const QString &filePath) const
{
    const QFileInfo sourceInfo(filePath);
    const QString fileName = sourceInfo.fileName();

    const QImage thumbnail = DirectoryImageCache::instance()->thumbnailForFileName(fileName);
    return thumbnail;
}

QUrl DirectoryModel::thumbnailUrlForFile(const QString &filePath) const
{
    const QFileInfo sourceInfo(filePath);
    if (!sourceInfo.exists())
        return QUrl::fromLocalFile(filePath);

    const QString fileName = sourceInfo.fileName();
    if (!DirectoryImageCache::instance()->hasThumbnail(fileName)) {
        DirectoryImageCache::instance()->generateThumbnails();
    }

    const QImage thumbnail = DirectoryImageCache::instance()->thumbnailForFileName(fileName);
    if (thumbnail.isNull()) {
        return QUrl::fromLocalFile(filePath);
    }

    const QString cachePath = thumbnailCachePath(filePath);
    const QFileInfo cacheInfo(cachePath);
    if (!cacheInfo.dir().exists()) {
        QDir().mkpath(cacheInfo.absolutePath());
    }

    if (!QFile::exists(cachePath) || QImageReader(cachePath).read().isNull()) {
        thumbnail.save(cachePath, "PNG");
    }

    return QUrl::fromLocalFile(cachePath);
}

QString DirectoryModel::thumbnailCachePath(const QString &filePath) const
{
    const QByteArray hash = QCryptographicHash::hash(filePath.toUtf8(), QCryptographicHash::Sha256).toHex();
    const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                             + QStringLiteral("/DiaBox/thumbnails");
    return QDir(cacheDir).absoluteFilePath(QStringLiteral("%1.png").arg(QString::fromUtf8(hash)));
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
    m_files = files;
    endResetModel();

    emit countChanged();
}
