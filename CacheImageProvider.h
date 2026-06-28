#pragma once

#include <QQuickImageProvider>
#include "DirectoryImageCache.h"

/**
 * CacheImageProvider – Qt Quick image provider for serving cached thumbnails.
 * Allows QML to load images via: image://cache/filename
 */
class CacheImageProvider : public QQuickImageProvider
{
public:
    explicit CacheImageProvider(DirectoryImageCache *cache);

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    DirectoryImageCache *m_cache;
};
