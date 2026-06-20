#pragma once

#include <QAbstractListModel>
#include <QFileSystemWatcher>
#include <QStringList>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

/**
 * DirectoryModel – watches a local folder and exposes all image files
 * as a QAbstractListModel.  Changes on disk are detected automatically
 * via QFileSystemWatcher and the model refreshes itself.
 */
class DirectoryModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString folder READ folder WRITE setFolder NOTIFY folderChanged)
    Q_PROPERTY(int     count  READ rowCount                NOTIFY countChanged)

public:
    enum Roles {
        FileNameRole = Qt::UserRole + 1,
        FilePathRole,
        FileUrlRole,
    };
    Q_ENUM(Roles)

    explicit DirectoryModel(QObject *parent = nullptr);

    // ── Q_PROPERTY accessors ─────────────────────────────────────────────────
    QString folder() const;
    void    setFolder(const QString &folder);

    // ── QAbstractListModel interface ─────────────────────────────────────────
    int     rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // ── Helpers callable from QML ────────────────────────────────────────────
    Q_INVOKABLE QUrl    fileUrl(int row)  const;
    Q_INVOKABLE QString fileName(int row) const;

signals:
    void folderChanged();
    void countChanged();

private slots:
    void onDirectoryChanged(const QString &path);

private:
    void refresh();

    QString             m_folder;
    QStringList         m_files;
    QFileSystemWatcher  m_watcher;

    static const QStringList s_extensions;
};
