#include "audio.h"
#include <client.h>
#include <QUrl>

namespace vk {

class AudioProvider;
class AudioProviderPrivate
{
    Q_DECLARE_PUBLIC(AudioProvider)
public:
    AudioProviderPrivate(AudioProvider *q, Client *client) : q_ptr(q), client(client) {}
    AudioProvider *q_ptr;
    Client *client;

    void _q_audio_received(const QVariant &response)
    {
        Q_Q(AudioProvider);
        foreach (auto item, response.toList()) {
            auto map = item.toMap();
            AudioItem audio(client);
            audio.setId(map.value("aid").toInt());
            audio.setOwnerId(map.value("owner_id").toInt());
            audio.setArtist(map.value("artist").toString());
            audio.setTitle(map.value("title").toString());
            audio.setDuration(map.value("duration").toReal());
            audio.setAlbumId(map.value("album").toInt());
            audio.setLyricsId(map.value("lyrics_id").toInt());
            audio.setUrl(map.value("url").toUrl());
            emit q->audioItemReceived(audio);
        }
    }
};

AudioProvider::AudioProvider(Client *client) :
    d_ptr(new AudioProviderPrivate(this, client))
{

}

AudioProvider::~AudioProvider()
{

}

/*!
 * \brief AudioProvider::get \link http://vk.com/developers.php?oid=-1&p=audio.get
 * \param uid
 * \param count
 * \param offset
 * \return reply
 */
Reply *AudioProvider::get(int uid, int count, int offset)
{
    Q_D(AudioProvider);
    QVariantMap args;
    args.insert(uid > 0 ? "uid" : "gid", qAbs(uid));
    args.insert("count", count);
    args.insert("offset", offset);

    auto reply = d->client->request("audio.get", args);
    connect(reply, SIGNAL(resultReady(QVariant)), SLOT(_q_audio_received(QVariant)));
    return reply;
}

class AudioModel;
class AudioModelPrivate
{
    Q_DECLARE_PUBLIC(AudioModel)
public:
    AudioModelPrivate(AudioModel *q) : q_ptr(q) {}
    AudioModel *q_ptr;
    AudioItemList itemList;
    Qt::SortOrder sortOrder;
};

AudioModel::AudioModel(QObject *parent) : QAbstractListModel(parent),
    d_ptr(new AudioModelPrivate(this))
{  
    auto roles = roleNames();
    roles[IdRole] = "aid";
    roles[TitleRole] = "title";
    roles[ArtistRole] = "artist";
    roles[UrlRole] = "url";
    roles[DurationRole] = "duration";
    roles[AlbumIdRole] = "albumId";
    roles[LyricsIdRole] = "lyricsId";
    roles[OwnerIdRole] = "ownerId";
    setRoleNames(roles);
}

AudioModel::~AudioModel()
{
}

int AudioModel::count() const
{
    return d_func()->itemList.count();
}

void AudioModel::insertAudio(int index, const AudioItem &item)
{
    beginInsertRows(QModelIndex(), index, index);
    d_func()->itemList.insert(index, item);
    endInsertRows();
}

void AudioModel::replaceAudio(int i, const AudioItem &item)
{
    auto index = createIndex(i, 0);
    d_func()->itemList[i] = item;
    emit dataChanged(index, index);
}

void AudioModel::sort(int, Qt::SortOrder order)
{
    Q_D(AudioModel);
    (order == Qt::AscendingOrder) ? qSort(d->itemList.begin(), d->itemList.end())
                                  : qSort(d->itemList.end(), d->itemList.begin());
    emit dataChanged(createIndex(0, 0), createIndex(count() - 1, 0));
}

void AudioModel::removeAudio(int aid)
{
    Q_D(AudioModel);
    int index = findAudio(aid);
    if (index == -1)
        return;
    beginRemoveRows(QModelIndex(), index, index);
    d->itemList.removeAt(index);
    endRemoveRows();
}

void AudioModel::addAudio(const AudioItem &item)
{
    Q_D(AudioModel);
    if (findAudio(item.id()) != -1)
        return;

    auto it = d->sortOrder == Qt::AscendingOrder ? qLowerBound(d->itemList.begin(),
                                                               d->itemList.end(),
                                                               item)
                                                 : qLowerBound(d->itemList.end(),
                                                               d->itemList.begin(),
                                                               item);
    auto index = it - d->itemList.begin();
    insertAudio(index, item);
}

void AudioModel::clear()
{
    Q_D(AudioModel);
    beginRemoveRows(QModelIndex(), 0, d->itemList.count());
    d->itemList.clear();
    endRemoveRows();
}

int AudioModel::rowCount(const QModelIndex &) const
{
    return count();
}

void AudioModel::setSortOrder(Qt::SortOrder order)
{
    Q_D(AudioModel);
    if (order != d->sortOrder) {
        d->sortOrder = order;
        emit sortOrderChanged(order);
        sort(0, d->sortOrder);
    }
}

Qt::SortOrder AudioModel::sortOrder() const
{
    return d_func()->sortOrder;
}

QVariant AudioModel::data(const QModelIndex &index, int role) const
{
    Q_D(const AudioModel);
    int row = index.row();
    auto item = d->itemList.at(row);
    switch (role) {
    case IdRole:
        return item.id();
        break;
    case TitleRole:
        return item.title();
    case ArtistRole:
        return item.artist();
    case UrlRole:
        return item.url();
    case DurationRole:
        return item.duration();
    case AlbumIdRole:
        return item.albumId();
    case LyricsIdRole:
        return item.lyricsId();
    case OwnerIdRole:
        return item.ownerId();
    default:
        break;
    }
    return QVariant::Invalid;
}

int AudioModel::findAudio(int id) const
{
    Q_D(const AudioModel);
    for (int i = 0; i != d->itemList.count(); i++)
        if (d->itemList.at(i).id() == id)
            return id;
    return -1;
}

} // namespace vk

#include "moc_audio.cpp"
