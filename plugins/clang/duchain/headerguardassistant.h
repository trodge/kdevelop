/*
 * Copyright 2018 Amish K. Naidu <amhndu@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef HEADERGUARDASSISTANT_H
#define HEADERGUARDASSISTANT_H

#include <serialization/indexedstring.h>
#include <interfaces/iassistant.h>

#include <clang-c/Index.h>

class HeaderGuardAssistant
    : public KDevelop::IAssistant
{
    Q_OBJECT

public:
    HeaderGuardAssistant(const CXTranslationUnit unit, const CXFile file);
    virtual ~HeaderGuardAssistant() override = default;

    QString title() const override;

    void createActions() override;

private:
    const int m_line;
    const KDevelop::IndexedString m_path;
};

#endif // HEADERGUARDASSISTANT_H
