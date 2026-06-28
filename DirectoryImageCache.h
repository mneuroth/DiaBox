#pragma once

#include <QDateTime>
#include <QHash>
#include <QImage>
#include <QList>
#include <QFileInfo>
#include <QString>
#include <QStringList>

class DirectoryImageCache
{
public:
    explicit DirectoryImageCache(const QString &directoryPath = QString());

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

private:
    struct CacheItem {
        QString   filePath;
        QImage    thumbnail;
        QDateTime lastModified;
    };

    void buildFileList(QList<QFileInfo> &fileInfos) const;
    QImage createThumbnail(const QString &filePath) const;
    static bool isImageFile(const QFileInfo &fileInfo);

    QString m_directoryPath;
    QHash<QString, CacheItem> m_cache;

    static const QStringList s_supportedExtensions;
};
