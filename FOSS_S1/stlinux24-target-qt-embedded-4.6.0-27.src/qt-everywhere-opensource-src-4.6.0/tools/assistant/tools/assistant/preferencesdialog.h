/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Assistant of the Qt Toolkit.
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

#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QtGui/QDialog>
#include "ui_preferencesdialog.h"

QT_BEGIN_NAMESPACE

class FontPanel;
class QHelpEngineCore;

enum {
    ShowHomePage = 0,
    ShowBlankPage = 1,
    ShowLastPages = 2
};

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    PreferencesDialog(QHelpEngineCore *helpEngine, QWidget *parent = 0);
    ~PreferencesDialog();

    void showDialog();

private slots:
    void updateAttributes(QListWidgetItem *item);
    void updateFilterMap();
    void addFilter();
    void removeFilter();
    void addDocumentationLocal();
    void removeDocumentation();
    void applyChanges();
    void appFontSettingToggled(bool on);
    void appFontSettingChanged(int index);
    void browserFontSettingToggled(bool on);
    void browserFontSettingChanged(int index);
    
    void setBlankPage();
    void setCurrentPage();
    void setDefaultPage();

signals:
    void updateBrowserFont();
    void updateApplicationFont();

private:
    void updateFilterPage();
    void updateFontSettingsPage();
    void updateOptionsPage();

    Ui::PreferencesDialogClass m_ui;
    QHelpEngineCore *m_helpEngine;
    bool m_hideFiltersTab;
    bool m_hideDocsTab;
    QMap<QString, QStringList> m_filterMapBackup;
    QMap<QString, QStringList> m_filterMap;
    QStringList m_removedFilters;
    QStringList m_docsBackup;
    QStringList m_regDocs;
    QStringList m_unregDocs;
    QList<int> m_TabsToClose;
    FontPanel *m_appFontPanel;
    FontPanel *m_browserFontPanel;
    bool m_appFontChanged;
    bool m_browserFontChanged;
};

QT_END_NAMESPACE

#endif // SETTINGSDIALOG_H
