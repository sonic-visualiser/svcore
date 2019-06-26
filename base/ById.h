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

#include "Debug.h"

#include <memory>
#include <map>
#include <typeinfo>
#include <iostream>
#include <climits>

#include <QMutex>
#include <QString>

#include "XmlExportable.h"

template <typename T>
struct SvId {
    
    int id;

    enum {
        // The value NO_ID (-1) is never allocated by WithId
        NO_ID = -1
    };
    
    SvId() : id(NO_ID) {}

    SvId(const SvId &) =default;
    SvId &operator=(const SvId &) =default;

    bool operator==(const SvId &other) const { return id == other.id; }
    bool operator<(const SvId &other) const { return id < other.id; }

    bool isNone() const { return id == NO_ID; }

    QString toString() const {
        return QString("%1").arg(id);
    }
};

template <typename T>
std::ostream &
operator<<(std::ostream &ostr, const SvId<T> &id)
{
    // For diagnostic purposes only. Do not use these IDs for
    // serialisation - see XmlExportable instead.
    if (id.isNone()) {
        return (ostr << "<none>");
    } else {
        return (ostr << "#" << id.id);
    }
}

template <typename T>
class WithId
{
public:
    typedef SvId<T> Id;
    
    WithId() :
        m_id(getNextId()) {
    }

    /**
     * Return an id for this object. The id is a unique identifier for
     * this object among all objects that implement WithId within this
     * single run of the application.
     */
    Id getId() const {
        Id id;
        id.id = m_id;
        return id;
    }

private:
    int m_id;

    static int getNextId() {
        static int nextId = 0;
        static QMutex mutex;
        QMutexLocker locker(&mutex);
        int i = nextId;
        if (nextId == INT_MAX) {
            nextId = INT_MIN;
        } else {
            ++nextId;
            if (nextId == 0 || nextId == Id::NO_ID) {
                throw std::runtime_error("Internal ID limit exceeded!");
            }
        }
        return i;
    }
};

template <typename Item, typename Id>
class ById
{
public:
    ~ById() {
        QMutexLocker locker(&m_mutex);
        for (const auto &p: m_items) {
            if (p.second && p.second.use_count() > 0) {
                SVCERR << "WARNING: ById map destroyed with use count of "
                       << p.second.use_count() << " for item with type "
                       << typeid(*p.second.get()).name()
                       << " and id " << p.first.id << endl;
            }
        }
    }
    
    void add(std::shared_ptr<Item> item) {
        QMutexLocker locker(&m_mutex);
        auto id = item->getId();
        if (id.isNone()) {
            throw std::logic_error("item id should never be None");
        }
        if (m_items.find(id) != m_items.end()) {
            SVCERR << "WARNING: ById::add: item with id " << id
                   << " is already recorded, replacing it (item type is "
                   << typeid(*item.get()).name() << ")" << endl;
        }
        m_items[id] = item;
    }

    void
    release(Id id) {
        QMutexLocker locker(&m_mutex);
        m_items.erase(id);
    }

    std::shared_ptr<Item> get(Id id) const {
        if (id.isNone()) return {}; // this id is never issued: avoid locking
        QMutexLocker locker(&m_mutex);
        const auto &itr = m_items.find(id);
        if (itr != m_items.end()) {
            return itr->second;
        } else {
            return {};
        }
    }

    template <typename Derived>
    std::shared_ptr<Derived> getAs(Id id) const {
        return std::dynamic_pointer_cast<Derived>(get(id));
    }

    /**
     * If the Item type is an XmlExportable, return the export ID of
     * the given item ID. A call to this function will fail to compile
     * if the Item is not an XmlExportable.
     *
     * The export ID is a simple int, and is only allocated when first
     * requested, so objects that are never exported don't get one.
     */
    int getExportId(Id id) const {
        auto exportable = getAs<XmlExportable>(id);
        if (exportable) {
            return exportable->getExportId();
        } else {
            return XmlExportable::NO_ID;
        }
    }
    
private:
    mutable QMutex m_mutex;
    std::map<Id, std::shared_ptr<Item>> m_items;
};

template <typename Item, typename Id>
class StaticById
{
public:
    static void add(std::shared_ptr<Item> imagined) {
        byId().add(imagined);
    }

    static void release(Id id) {
        byId().release(id);
    }

    static std::shared_ptr<Item> get(Id id) {
        return byId().get(id);
    }

    template <typename Derived>
    static
    std::shared_ptr<Derived> getAs(Id id) {
        return std::dynamic_pointer_cast<Derived>(get(id));
    }

    static int getExportId(Id id) {
        return byId().getExportId(id);
    }
    
private:
    static
    ById<Item, Id> &byId() {
        static ById<Item, Id> b;
        return b;
    }
};

#endif
