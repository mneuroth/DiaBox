#pragma once

#include <QDateTime>
#include <QFileSystemWatcher>
#include <QHash>
#include <QImage>
#include <QList>
#include <QFileInfo>
#include <QObject>
#include <QString>
#include <QStringList>

class DirectoryImageCache : public QObject
{
    Q_OBJECT

public:
    explicit DirectoryImageCache(const QString &directoryPath = QString(), QObject *parent = nullptr);

    void setDirectoryPath(const QString &directoryPath);
    QString directoryPath() const;

    void generateThumbnails();
    bool update();

    bool save(const QString &cacheFilePath) const;
    bool load(const QString &cacheFilePath);

    QImage thumbnailForFileName(const QString &fileName) const;
    bool hasThumbnail(const QString &fileName) const;
    QStringList imageFileNames() const;
    void clear();

private slots:
    void onDirectoryChanged(const QString &path);

private:
    struct CacheItem {
        QString   filePath;
        QImage    thumbnail;
        QDateTime lastModified;
    };

    void buildFileList(QList<QFileInfo> &fileInfos) const;
    QImage createThumbnail(const QString &filePath) const;
    static bool isImageFile(const QFileInfo &fileInfo);
    void updateWatcher();

    QString m_directoryPath;
    QHash<QString, CacheItem> m_cache;
    QFileSystemWatcher m_watcher;

    static const QStringList s_supportedExtensions;
};
