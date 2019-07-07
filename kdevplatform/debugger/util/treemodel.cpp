/*
 * This file is part of KDevelop
 *
 * Copyright 2008 Vladimir Prus <ghost@cs.msu.su>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "treemodel.h"

#include <iostream>

#include "treeitem.h"

using namespace KDevelop;

class KDevelop::TreeModelPrivate
{
public:
    explicit TreeModelPrivate(const QVector<QString>& headers)
        : headers(headers)
    {
    }

    QVector<QString> headers;
    TreeItem* root = nullptr;
};

TreeModel::TreeModel(const QVector<QString>& headers,
                     QObject *parent)
  : QAbstractItemModel(parent)
  , d_ptr(new TreeModelPrivate(headers))
{
}

void TreeModel::setRootItem(TreeItem *item)
{
    Q_D(TreeModel);

    d->root = item;
    d->root->fetchMoreChildren();
}

TreeModel::~TreeModel()
{
    Q_D(TreeModel);

    delete d->root;
}

int TreeModel::columnCount(const QModelIndex &parent) const
{
    Q_D(const TreeModel);

    Q_UNUSED(parent);
    return d->headers.size();
}

QVariant TreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    auto *item = static_cast<TreeItem*>(index.internalPointer());
    if (role == ItemRole)
        return QVariant::fromValue(item);

    return item->data(index.column(), role);
}

Qt::ItemFlags TreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    Q_D(const TreeModel);

    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return d->headers.value(section);

    return QVariant();
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent)
    const
{
    Q_D(const TreeModel);

    if (!hasIndex(row, column, parent))
        return QModelIndex();

    TreeItem *parentItem;

    if (!parent.isValid())
        parentItem = d->root;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    TreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return createIndex(row, column, nullptr);
}

QModelIndex TreeModel::parent(const QModelIndex &index) const
{
    Q_D(const TreeModel);

    if (!index.isValid())
        return QModelIndex();

    auto *childItem = static_cast<TreeItem*>(index.internalPointer());
    TreeItem *parentItem = childItem->parent();

    if (parentItem == d->root)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int TreeModel::rowCount(const QModelIndex &parent) const
{
    Q_D(const TreeModel);

    TreeItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = d->root;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    if(parentItem)
        return parentItem->childCount();
    else
        return 0;
}

TreeItem* TreeModel::itemForIndex(const QModelIndex& index) const
{
    Q_D(const TreeModel);

    if (!index.isValid())
        return d->root;
    else
        return static_cast<TreeItem*>(index.internalPointer());
}

QModelIndex TreeModel::indexForItem(TreeItem *item, int column) const
{
    if (item->parent() == nullptr)
        return QModelIndex();

    if (TreeItem* parent = item->parent())
    {
        /* FIXME: we might store row directly in item.  */
        int row = parent->childItems.indexOf(item);
        Q_ASSERT(row != -1);

        return createIndex(row, column, item);
    }
    else
    {
        return QModelIndex();
    }
}

void TreeModel::expanded(const QModelIndex &index)
{
    TreeItem* item = itemForIndex(index);
    QObject::connect(item, &TreeItem::allChildrenFetched, this, &TreeModel::itemChildrenReady);
    if (item->hasMore() && item->childCount() == 1)
        item->fetchMoreChildren();
    else
        emit itemChildrenReady();
    item->setExpanded(true);
}

void TreeModel::collapsed(const QModelIndex &index)
{
    TreeItem* item = itemForIndex(index);
    item->setExpanded(false);
}

void TreeModel::clicked(const QModelIndex &index)
{
    TreeItem* item = itemForIndex(index);
    item->clicked();
}

bool TreeModel::setData(const QModelIndex& index, const QVariant& value,
                        int role)
{
    /* FIXME: CheckStateRole is dirty.  Should we pass the role to
       the item?  */
    if (index.isValid()
        && (role == Qt::EditRole || role == Qt::CheckStateRole))
    {

        auto *item = static_cast<TreeItem*>(index.internalPointer());
        item->setColumn(index.column(), value);
        return true;
    }
    return false;
}

KDevelop::TreeItem* KDevelop::TreeModel::root() const
{
    Q_D(const TreeModel);

    return d->root;
}


