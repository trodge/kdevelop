/* KDevelop
 *
 * Copyright 2011 Aleix Pol <aleixpol@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

import QtQuick 2.7

Rectangle {
    id: root

    SystemPalette {
        id: myPalette
    }

    color: myPalette.window

    Loader {
        id: loader

        anchors.fill: parent

        // non-code areas are broken ATM, so just go blank for them
        // old: source: "qrc:///qml/area_"+area+".qml"
        source: area === "code" ? "qrc:///qml/area_code.qml" : ""
        asynchronous: true
        opacity: status === Loader.Ready

        Behavior on opacity {
            PropertyAnimation {}
        }
    }

}
