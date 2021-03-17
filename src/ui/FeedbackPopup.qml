/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of UsageStatistic plugin for Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Styles 1.4
import QtQuick.Window 2.15

Rectangle {
    id: root_rectangle

    property int rating: 0

    signal submitFeedback(string feedback, int rating)
    signal closeClicked()

    width: 740
    height: 382
    border { color: "#0094ce"; width: 1 }

    Text {
        id: h1
        objectName: "title"
        color: "#333333"
        text: "Enjoying Qt Design Studio?"
        font { family: "Titillium"; capitalization: Font.AllUppercase; pixelSize: 21 }
        anchors { horizontalCenter: parent.horizontalCenter; top: parent.top; topMargin: 50 }
    }

    Text {
        id: h2
        color: "#333333"
        text: "Tap a star to rate the usefulness of the feature"
        font { family: "Titillium"; pixelSize: 21 }
        anchors { horizontalCenter: parent.horizontalCenter; top: h1.bottom; topMargin: 12 }
    }

    Row {
        id: starRow
        width: 246; height: 42; spacing: 6.5
        anchors { horizontalCenter: parent.horizontalCenter; top: h2.bottom; topMargin: 32 }

        Repeater { // create the stars
            id: rep
            model: 5
            Image {
                source: "../images/star_empty.png"
                fillMode: Image.PreserveAspectFit

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        for (var i = 0; i < 5; ++i) {
                            rep.itemAt(i).source = i <= index ? "../images/star_filled.png"
                                                              : "../images/star_empty.png"
                        }
                        rating = index + 1
                    }
                }
            }
        }
    }

    ScrollView {
        id: scroll_textarea
        width: 436
        height: 96
        anchors { horizontalCenter: parent.horizontalCenter; top: starRow.bottom; topMargin: 28 }

        TextArea {
            id: textarea
            width: 426
            height: 90
            color: "#333333";
            placeholderText: "We highly appreciate additional feedback.\nBouquets or Brickbats, suggestions, all welcome!"
            font { pixelSize: 14; family: "Titillium" }
            wrapMode: Text.Wrap
        }

        background: Rectangle {
            border { color: "#e6e6e6"; width: 1 }
        }
    }

    Row {
        id: buttonRow
        anchors { horizontalCenter: parent.horizontalCenter; top: scroll_textarea.bottom; topMargin: 28 }
        spacing: 10

        Button {
            id: buttonSkip
            width: 80
            height: 28

            contentItem: Text {
                text: "Skip"
                color: parent.hovered ? Qt.darker("#999999", 1.9) : Qt.darker("#999999", 1.2)
                font { family: "Titillium"; pixelSize: 14 }
                horizontalAlignment: Text.AlignHCenter
            }

            background: Rectangle {
                anchors.fill: parent
                color: "#ffffff"
                border { color: "#999999"; width: 1 }
            }

            onClicked: root_rectangle.closeClicked()
        }

        Button {
            id: buttonSubmit

            width: 80
            height: 28
            enabled: rating > 0

            contentItem: Text {
                text: "Submit";
                color: enabled ? "white" : Qt.lighter("#999999", 1.3)
                font { family: "Titillium"; pixelSize: 14 }
                horizontalAlignment: Text.AlignHCenter
            }

            background: Rectangle {
                anchors.fill: parent
                color: enabled ? parent.hovered ? Qt.lighter("#0094ce", 1.2) : "#0094ce" : "white"
                border { color: enabled ? "#999999" : Qt.lighter("#999999", 1.3); width: 1 }
            }

            onClicked: {
                root_rectangle.submitFeedback(textarea.text, rating);
                root_rectangle.closeClicked();
            }
        }
    }
}
