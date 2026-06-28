#include "CacheImageProvider.h"

CacheImageProvider::CacheImageProvider(DirectoryImageCache *cache)
    : QQuickImageProvider(QQuickImageProvider::Image)
    , m_cache(cache)
{
}

QImage CacheImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    if (!m_cache)
        return QImage();

    // id = fileName (e.g., "image.jpg")
    QImage image = m_cache->thumbnailForFileName(id);

    if (image.isNull())
        return QImage();

    if (size)
        *size = image.size();

    if (requestedSize.isValid())
        return image.scaledToWidth(requestedSize.width(), Qt::SmoothTransformation);

    return image;
}
