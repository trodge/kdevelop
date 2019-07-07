/* This file is part of KDevelop
    Copyright 2002-2005 Roberto Raggi <roberto@kdevelop.org>
    Copyright 2006 Adam Treat <treat@kde.org>
    Copyright 2006 Hamish Rodda <rodda@kde.org>
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

#ifndef KDEVPLATFORM_CLASSMEMBERDECLARATION_H
#define KDEVPLATFORM_CLASSMEMBERDECLARATION_H

#include "declaration.h"

namespace KDevelop {
class ClassMemberDeclarationData;
/**
 * Represents a single class member definition in a definition-use chain.
 */
class KDEVPLATFORMLANGUAGE_EXPORT ClassMemberDeclaration
    : public Declaration
{
public:
    ClassMemberDeclaration(const ClassMemberDeclaration& rhs);
    ClassMemberDeclaration(const RangeInRevision& range, DUContext* context);
    explicit ClassMemberDeclaration(ClassMemberDeclarationData& dd);
    ~ClassMemberDeclaration() override;

    ClassMemberDeclaration& operator=(const ClassMemberDeclaration& rhs) = delete;

    AccessPolicy accessPolicy() const;
    void setAccessPolicy(AccessPolicy accessPolicy);

    enum StorageSpecifier {
        StaticSpecifier   = 0x1 /**< indicates static member */,
        AutoSpecifier     = 0x2 /**< indicates automatic determination of member access */,
        FriendSpecifier   = 0x4 /**< indicates friend member */,
        ExternSpecifier   = 0x8 /**< indicates external declaration */,
        RegisterSpecifier = 0x10 /**< indicates register */,
        MutableSpecifier  = 0x20/**< indicates a mutable member */
    };
    Q_DECLARE_FLAGS(StorageSpecifiers, StorageSpecifier)

    void setStorageSpecifiers(StorageSpecifiers specifiers);

    bool isStatic() const;
    void setStatic(bool isStatic);

    bool isAuto() const;
    void setAuto(bool isAuto);

    bool isFriend() const;
    void setFriend(bool isFriend);

    bool isRegister() const;
    void setRegister(bool isRegister);

    bool isExtern() const;
    void setExtern(bool isExtern);

    bool isMutable() const;
    void setMutable(bool isMutable);

    /**
     * \returns The size in bytes or -1 if unknown.
     */
    int64_t sizeOf() const;

    /**
     * Set the size to given number of bytes. Use -1 to represent unknown size.
     */
    void setSizeOf(int64_t sizeOf);

    /**
     * \returns The offset of the field in bits or -1 if unknown or not applicable.
     */
    int64_t bitOffsetOf() const;

    /**
     * Set the offset to given number of bits. Use -1 to represent unknown offset.
     */
    void setBitOffsetOf(int64_t bitOffsetOf);

    /**
     * \returns The alignment in bytes or -1 if unknown.
     */
    int64_t alignOf() const;

    /**
     * Set the alignment to given number of bytes.
     *
     * The value must be non-negative power of 2 or -1 to represent unknown alignment.
     */
    void setAlignOf(int64_t alignedTo);

    enum {
        Identity = 9
    };

protected:
    ClassMemberDeclaration(ClassMemberDeclarationData& dd, const RangeInRevision& range);

    DUCHAIN_DECLARE_DATA(ClassMemberDeclaration)

private:
    Declaration* clonePrivate() const override;
};
}

Q_DECLARE_OPERATORS_FOR_FLAGS(KDevelop::ClassMemberDeclaration::StorageSpecifiers)

#endif // KDEVPLATFORM_CLASSMEMBERDECLARATION_H
