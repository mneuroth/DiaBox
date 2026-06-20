#include "DirectoryModel.h"

#include <QDir>
#include <QFileInfo>
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

    switch (role) {
    case FileNameRole:
    case Qt::DisplayRole:
        return name;
    case FilePathRole:
        return QDir(m_folder).absoluteFilePath(name);
    case FileUrlRole:
        return QUrl::fromLocalFile(QDir(m_folder).absoluteFilePath(name));
    }
    return {};
}

QHash<int, QByteArray> DirectoryModel::roleNames() const
{
    return {
        { FileNameRole, "fileName" },
        { FilePathRole, "filePath" },
        { FileUrlRole,  "fileUrl"  },
    };
}

// ── Q_INVOKABLE helpers ───────────────────────────────────────────────────────

QUrl DirectoryModel::fileUrl(int row) const
{
    if (row < 0 || row >= m_files.size())
        return {};
    return QUrl::fromLocalFile(QDir(m_folder).absoluteFilePath(m_files.at(row)));
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
