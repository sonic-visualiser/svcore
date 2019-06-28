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
#include <iostream>
#include <climits>
#include <stdexcept>

#include <QMutex>
#include <QString>

#include "XmlExportable.h"

struct IdAlloc {

    // The value NO_ID (-1) is never allocated
    static const int NO_ID = -1;
    
    static int getNextId();
};

template <typename T>
struct TypedId {
    
    int untyped;
    
    TypedId() : untyped(IdAlloc::NO_ID) {}

    TypedId(const TypedId &) =default;
    TypedId &operator=(const TypedId &) =default;

    bool operator==(const TypedId &other) const {
        return untyped == other.untyped;
    }
    bool operator!=(const TypedId &other) const {
        return untyped != other.untyped;
    }
    bool operator<(const TypedId &other) const {
        return untyped < other.untyped;
    }
    bool isNone() const {
        return untyped == IdAlloc::NO_ID;
    }
};

template <typename T>
std::ostream &
operator<<(std::ostream &ostr, const TypedId<T> &id)
{
    // For diagnostic purposes only. Do not use these IDs for
    // serialisation - see XmlExportable instead.
    if (id.isNone()) {
        return (ostr << "<none>");
    } else {
        return (ostr << "#" << id.untyped);
    }
}

class WithId
{
public:
    WithId() :
        m_id(IdAlloc::getNextId()) {
    }
    virtual ~WithId() {
    }

    /**
     * Return an id for this object. The id is a unique number for
     * this object among all objects that implement WithId within this
     * single run of the application.
     */
    int getUntypedId() const {
        return m_id;
    }

private:
    int m_id;
};

template <typename T>
class WithTypedId : virtual public WithId
{
public:
    typedef TypedId<T> Id;
    
    WithTypedId() : WithId() { }

    /**
     * Return an id for this object. The id is a unique value for this
     * object among all objects that implement WithTypedId within this
     * single run of the application.
     */
    Id getId() const {
        Id id;
        id.untyped = getUntypedId();
        return id;
    }
};

class AnyById
{
public:
    static void add(int, std::shared_ptr<WithId>);
    static void release(int);
    static std::shared_ptr<WithId> get(int);

    template <typename Derived>
    static std::shared_ptr<Derived> getAs(int id) {
        std::shared_ptr<WithId> p = get(id);
        return std::dynamic_pointer_cast<Derived>(p);
    }

private:
    class Impl;
    static Impl &impl();
};

template <typename Item, typename Id>
class TypedById
{
public:
    static void add(std::shared_ptr<Item> item) {
        auto id = item->getId();
        if (id.isNone()) {
            throw std::logic_error("item id should never be None");
        }
        AnyById::add(id.untyped, item);
    }

    static void release(Id id) {
        AnyById::release(id.untyped);
    }
    static void release(std::shared_ptr<Item> item) {
        release(item->getId());
    }

    template <typename Derived>
    static std::shared_ptr<Derived> getAs(Id id) {
        if (id.isNone()) return {}; // this id is never issued: avoid locking
        return AnyById::getAs<Derived>(id.untyped);
    }

    static std::shared_ptr<Item> get(Id id) {
        return getAs<Item>(id);
    }

    /**
     * If the Item type is an XmlExportable, return the export ID of
     * the given item ID. A call to this function will fail to compile
     * if the Item is not an XmlExportable.
     *
     * The export ID is a simple int, and is only allocated when first
     * requested, so objects that are never exported don't get one.
     */
    static int getExportId(Id id) {
        auto exportable = getAs<XmlExportable>(id);
        if (exportable) {
            return exportable->getExportId();
        } else {
            return XmlExportable::NO_ID;
        }
    }
};

#endif

