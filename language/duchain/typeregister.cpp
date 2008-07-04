/* This file is part of KDevelop
    Copyright 2008 David Nolden <david.nolden.kdevelop@art-master.de>

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

#include "typeregister.h"

namespace KDevelop {
AbstractType* TypeSystem::create(AbstractTypeData* data) const {
  if(m_factories.size() <= data->typeClassId || m_factories[data->typeClassId] == 0)
    return 0;
  return m_factories[data->typeClassId]->create(data);
}

void TypeSystem::callDestructor(AbstractTypeData* data) const {
  if(m_factories.size() <= data->typeClassId || m_factories[data->typeClassId] == 0)
    return;
  return m_factories[data->typeClassId]->callDestructor(data);
}

uint TypeSystem::dynamicSize(const AbstractTypeData& data) const {
  if(m_factories.size() <= data.typeClassId || m_factories[data.typeClassId] == 0)
    return 0;
  return m_factories[data.typeClassId]->dynamicSize(data);
}

size_t TypeSystem::dataClassSize(const AbstractTypeData& data) const {
  if(m_dataClassSizes.size() <= data.typeClassId || m_dataClassSizes[data.typeClassId] == 0)
    return 0;
  return m_dataClassSizes[data.typeClassId];
}


void TypeSystem::copy(const AbstractTypeData& from, AbstractTypeData& to, bool constant) const {
  if(m_factories.size() <= from.typeClassId || m_factories[from.typeClassId] == 0) {
    Q_ASSERT(0); //Shouldn't try to copy an unknown type
    return;
  }
  return m_factories[from.typeClassId]->copy(from, to, constant);
}

TypeSystem& TypeSystem::self() {
  static TypeSystem system;
  return system;
}
}
