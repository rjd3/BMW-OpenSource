/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QApplication>
#include <QString>
#include <QFile>

#include "stylewidget.h"



StyleWidget::StyleWidget(QWidget *parent)
 : QFrame(parent)
{
	m_ui.setupUi(this);
}


void StyleWidget::on_close_clicked()
{
    close();
}

void StyleWidget::on_blueStyle_clicked()
{
    QFile styleSheet(":/files/blue.qss");

    if (!styleSheet.open(QIODevice::ReadOnly)) {
        qWarning("Unable to open :/files/blue.qss");
        return;
    }

    qApp->setStyleSheet(styleSheet.readAll());
}

void StyleWidget::on_khakiStyle_clicked()
{
    QFile styleSheet(":/files/khaki.qss");

    if (!styleSheet.open(QIODevice::ReadOnly)) {
        qWarning("Unable to open :/files/khaki.qss");
        return;
    }

    qApp->setStyleSheet(styleSheet.readAll());
}


void StyleWidget::on_noStyle_clicked()
{
    QFile styleSheet(":/files/nostyle.qss");

    if (!styleSheet.open(QIODevice::ReadOnly)) {
        qWarning("Unable to open :/files/nostyle.qss");
        return;
    }

    qApp->setStyleSheet(styleSheet.readAll());
}


void StyleWidget::on_transparentStyle_clicked()
{
    QFile styleSheet(":/files/transparent.qss");

    if (!styleSheet.open(QIODevice::ReadOnly)) {
        qWarning("Unable to open :/files/transparent.qss");
        return;
    }

    qApp->setStyleSheet(styleSheet.readAll());
}



