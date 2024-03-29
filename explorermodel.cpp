#include "explorermodel.h"
#include <QFile>

ExplorerModel::ExplorerModel(QObject *parent) :
    QAbstractListModel(parent)
{
    QHash<int, QByteArray> roles;
    roles.insert(rFileInfo, "fileInfo");
    setRoleNames(roles);


    m_suffixFilter = QImageReader::supportedImageFormats();
    //m_suffixFilter << "bmp" << "jpg" << "jpeg" << "png";

    m_dir.setSorting(QDir::DirsFirst | QDir::Name);

    m_mode = Folders;
    m_path = "#";
}

ExplorerModel *ExplorerModel::instance()
{
    static ExplorerModel *_instance = 0;
    if (!_instance) {
        _instance = new ExplorerModel();
    }

    return _instance;
}

QVariant ExplorerModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    int row = index.row();

    switch (role) {
    case rFileInfo: {
        return QVariant::fromValue(m_filesList.at(row));
    } break;
    default:
        break;

    };

    return QVariant();
}

int ExplorerModel::rowCount(const QModelIndex &parent) const
{
    return m_filesList.count();
}

void ExplorerModel::update()
{
    emit beginUpdate();
    beginResetModel();

    clear(false);

    QFileInfo fileInfo;
    QFileInfoList list;

    switch (m_mode) {
    case Drivers: {
        list = m_dir.drives();
        setPath("");
    } break;
    case Folders: {
        list = m_dir.entryInfoList();
        setPath(m_dir.absolutePath());
    } break;
    }

    for (int i = 0; i < list.count(); ++i) {
        fileInfo = list.at(i);
        if (fileInfo.isDir() || m_suffixFilter.contains(fileInfo.suffix().toLower().toAscii())) {
            if (fileInfo.fileName() != "." && fileInfo.fileName() != "..") {
                FileInfo *fi;
                QString filePath = fileInfo.absoluteFilePath();
                if (m_selectedCache.contains(filePath)) {
                    fi = m_selectedCache.value(filePath);
                } else {
                    fi = new FileInfo(fileInfo);
                }

                fi->setIsDrive(m_mode == Drivers);
                m_filesList.append(fi);
            }
        }
    }

    endResetModel();
    emit endUpdate();
}

void ExplorerModel::clear(bool notSelected)
{
    FileInfo *fi;
    for (int i = 0; i < m_filesList.count(); ++i) {
        fi = m_filesList.at(i);
        if (!notSelected && !fi->selected())
            delete fi;
    }

    m_filesList.clear();
}

void ExplorerModel::drives()
{
    m_mode = Drivers;
    update();
}

void ExplorerModel::changeDir(FileInfo *fileInfo)
{
    m_mode = Folders;
    if (fileInfo->info()->isDir()) {
        m_dir.cd(fileInfo->info()->absoluteFilePath());
        update();
    }
}

bool ExplorerModel::changePath(QString path)
{
    bool result;
    if (path.isEmpty()) {
        drives();
        result = true;
    } else {
        if (m_dir.exists(path)) {
            m_dir.setPath(path);
            update();
            result = true;
        } else {
            result = false;
        }
    }

    return result;
}

void ExplorerModel::goUp()
{
    if (m_mode != Drivers) {
        if (m_dir.cdUp())
            update();
        else
            drives();
    }
}

void ExplorerModel::setPath(QString path)
{
    if (m_path != path) {
        m_path = path;
        emit pathChanged(path);
        emit dirChanged(path.right(path.count() - path.lastIndexOf("/") - 1));
    }
}

QString ExplorerModel::path() const
{
    return m_path;
}

void ExplorerModel::changeSelected(FileInfo *fi)
{
    if (!fi->isDir()) {
        if (!fi->selected()) {
            fi->setSelected(true);
            m_selectedCache.insert(fi->info()->absoluteFilePath(), fi);
        } else {
            fi->setSelected(false);
            m_selectedCache.remove(fi->info()->absoluteFilePath());
        }
    }

    emit selectedCountChanged(m_selectedCache.count());
}

int ExplorerModel::selectedCount() const
{
    return m_selectedCache.count();
}

void ExplorerModel::selectCurrent()
{
    FileInfo *fi;
    for (int i = 0; i < m_filesList.count(); ++i) {
        fi = m_filesList.at(i);
        if (!fi->isDir()) {
            fi->setSelected(true);
            m_selectedCache.insert(fi->info()->absoluteFilePath(), fi);
        }
    }
}

void ExplorerModel::deselectCurrent()
{
    FileInfo *fi;
    for (int i = 0; i < m_filesList.count(); ++i) {
        fi = m_filesList.at(i);
        if (!fi->isDir()) {
            fi->setSelected(false);
            m_selectedCache.remove(fi->info()->absoluteFilePath());
        }
    }
}

void ExplorerModel::clearSelected()
{
    FileInfo *fi;
    QHash<QString, FileInfo *>::iterator it;
    for (it = m_selectedCache.begin(); it != m_selectedCache.end(); ++it) {
        fi = it.value();
        if (!m_filesList.contains(fi))
            delete fi;
        else
            fi->setSelected(false);
    }
}

void ExplorerModel::copySelected(QString path)
{
    FileInfo *fi;
    int count = m_selectedCache.count();
    int copedCount = 0;
    emit progressChanged(0);
    QHash<QString, FileInfo *>::iterator it;
    for (it = m_selectedCache.begin(); it != m_selectedCache.end(); ++it) {
        fi = it.value();
        QFile::copy(fi->info()->absoluteFilePath(), path + "\\" + fi->name());
        ++copedCount;
        emit progressChanged(copedCount * 100 / count);
    }
}

void ExplorerModel::deleteSelected()
{
    FileInfo *fi;
    int count = m_selectedCache.count();
    int deletedCount = 0;
    emit progressChanged(0);
    QHash<QString, FileInfo *>::iterator it;
    for (it = m_selectedCache.begin(); it != m_selectedCache.end(); ++it) {
        fi = it.value();
        QFile::remove(fi->info()->absoluteFilePath());
        ++deletedCount;
        emit progressChanged(deletedCount * 100 / count);
    }
    qApp->processEvents();
    m_selectedCache.clear();
    clear();
    changePath(m_dir.path());
}

void ExplorerModel::showSelected()
{
    emit beginUpdate();
    beginResetModel();

    clear(false);
    QHash<QString, FileInfo *>::iterator it;
    for (it = m_selectedCache.begin(); it != m_selectedCache.end(); ++it) {
        m_filesList.append(it.value());
    }

    endResetModel();
    emit endUpdate();
}
