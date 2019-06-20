/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef SV_BY_ID_H
#define SV_BY_ID_H

#include <memory>
#include <map>

#include <QMutex>

typedef int Id;

class WithId
{
public:
    WithId() :
        m_id(getNextId()) {
    }

    Id getId() const {
        return m_id;
    }

private:
    Id m_id;
    static int getNextId();
};

template <typename Item>
class ById
{
public:
    void add(std::shared_ptr<Item> item) {
        QMutexLocker locker(&m_mutex);
        m_items[item->getId()] = item;
    }

    void
    release(Id id) {
        QMutexLocker locker(&m_mutex);
        m_items.erase(id);
    }

    std::shared_ptr<Item> get(Id id) const {
        QMutexLocker locker(&m_mutex);
        const auto &itr = m_items.find(id);
        if (itr != m_items.end()) {
            return itr->second;
        } else {
            return std::shared_ptr<Item>();
        }
    }

    template <typename Derived>
    std::shared_ptr<Derived> getAs(Id id) const {
        return std::dynamic_pointer_cast<Derived>(get(id));
    }
    
private:
    mutable QMutex m_mutex;
    std::map<Id, std::shared_ptr<Item>> m_items;
};
/*
class Imagined : public WithId {
};

class ImaginedById
{
public:
    static void add(std::shared_ptr<Imagined> imagined) {
        m_byId.add(imagined);
    }

    static void release(Id id) {
        m_byId.release(id);
    }

    static std::shared_ptr<Imagined> get(Id id) {
        return m_byId.get(id);
    }

    template <typename Derived>
    static
    std::shared_ptr<Derived> getAs(Id id) {
        return m_byId.getAs<Derived>(id);
    }

private:
    static ById<Imagined> m_byId;
};
*/
#endif

