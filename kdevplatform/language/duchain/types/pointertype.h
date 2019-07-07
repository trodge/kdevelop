/* This file is part of KDevelop
    Copyright 2006 Roberto Raggi <roberto@kdevelop.org>
    Copyright 2006-2008 Hamish Rodda <rodda@kde.org>
    Copyright 2007-2008 David Nolden <david.nolden.kdevelop@art-master.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
 */

#ifndef KDEVPLATFORM_POINTERTYPE_H
#define KDEVPLATFORM_POINTERTYPE_H

#include "abstracttype.h"

namespace KDevelop {
class PointerTypeData;

/**
 * \short A type representing pointer types.
 *
 * PointerType is used to represent types which hold a pointer to a location
 * in memory.
 */
class KDEVPLATFORMLANGUAGE_EXPORT PointerType
    : public AbstractType
{
public:
    using Ptr = TypePtr<PointerType>;

    /// Default constructor
    PointerType ();
    /// Copy constructor. \param rhs type to copy
    PointerType(const PointerType& rhs);
    /// Constructor using raw data. \param data internal data.
    explicit PointerType(PointerTypeData& data);
    /// Destructor
    ~PointerType() override;

    PointerType& operator=(const PointerType& rhs) = delete;

    /**
     * Sets the base type of the pointer, ie. what type of data the pointer points to.
     *
     * \param type the base type.
     */
    void setBaseType(const AbstractType::Ptr& type);

    /**
     * Retrieve the base type of the pointer, ie. what type of data the pointer points to.
     *
     * \returns the base type.
     */
    AbstractType::Ptr baseType () const;

    QString toString() const override;

    uint hash() const override;

    WhichType whichType() const override;

    AbstractType* clone() const override;

    bool equals(const AbstractType* rhs) const override;

    void exchangeTypes(TypeExchanger* exchanger) override;

    enum {
        Identity = 3
    };

    using Data = PointerTypeData;

protected:
    void accept0 (TypeVisitor* v) const override;

    TYPE_DECLARE_DATA(PointerType)
};

template <>
inline PointerType* fastCast<PointerType*>(AbstractType* from)
{
    if (!from || from->whichType() != AbstractType::TypePointer)
        return nullptr;
    else
        return static_cast<PointerType*>(from);
}
}

#endif
