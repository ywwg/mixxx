/***************************************************************************
                          dlgprefplaylist.cpp  -  description
                             -------------------
    begin                : Thu Apr 17 2003
    copyright            : (C) 2003 by Tue & Ken Haste Andersen
    email                : haste@diku.dk
***************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QStringList>
#include <QUrl>

#include "dlgprefplaylist.h"
#include "soundsourceproxy.h"

#define MIXXX_ADDONS_URL "http://www.mixxx.org/wiki/doku.php/add-ons"


DlgPrefPlaylist::DlgPrefPlaylist(QWidget * parent,
        ConfigObject<ConfigValue> * config, Library *pLibrary)
        :DlgPreferencePage(parent),
        m_dirListModel(),
        m_pconfig(config),
        m_pLibrary(pLibrary) {
    setupUi(this);
    slotUpdate();
    //Owen edit: my workaround works well enough for my personal use
    //checkbox_ID3_sync->setVisible(false);

    connect(this, SIGNAL(requestAddDir(QString)),
            m_pLibrary, SLOT(slotRequestAddDir(QString)));
    connect(this, SIGNAL(requestRemoveDir(QString, bool)),
            m_pLibrary, SLOT(slotRequestRemoveDir(QString, bool)));
    connect(this, SIGNAL(requestRelocateDir(QString,QString)),
            m_pLibrary, SLOT(slotRequestRelocateDir(QString,QString)));
    connect(PushButtonAddDir, SIGNAL(clicked()),
            this, SLOT(slotAddDir()));
    connect(PushButtonRemoveDir, SIGNAL(clicked()),
            this, SLOT(slotRemoveDir()));
    connect(PushButtonRelocateDir, SIGNAL(clicked()),
            this, SLOT(slotRelocateDir()));
    //connect(pushButtonM4A, SIGNAL(clicked()), this, SLOT(slotM4ACheck()));
    connect(pushButtonExtraPlugins, SIGNAL(clicked()),
            this, SLOT(slotExtraPlugins()));

    // plugins are loaded in src/main.cpp way early in boot so this is safe
    // here, doesn't need done at every slotUpdate
    QStringList plugins(SoundSourceProxy::supportedFileExtensionsByPlugins());
    if (plugins.length() > 0) {
        pluginsLabel->setText(plugins.join(", "));
    }
}

DlgPrefPlaylist::~DlgPrefPlaylist() {
}

void DlgPrefPlaylist::initialiseDirList(){
    // save which index was selected
    const QString selected = dirList->currentIndex().data().toString();
    // clear and fill model
    m_dirListModel.clear();
    QStringList dirs = m_pLibrary->getDirs();
    foreach (QString dir, dirs) {
        m_dirListModel.appendRow(new QStandardItem(dir));
    }
    dirList->setModel(&m_dirListModel);
    dirList->setCurrentIndex(m_dirListModel.index(0, 0));
    // reselect index if it still exists
    for (int i=0 ; i<m_dirListModel.rowCount() ; ++i) {
        const QModelIndex index = m_dirListModel.index(i, 0);
        if (index.data().toString() == selected) {
            dirList->setCurrentIndex(index);
            break;
        }
    }
}

void DlgPrefPlaylist::slotExtraPlugins() {
    QDesktopServices::openUrl(QUrl(MIXXX_ADDONS_URL));
}

void DlgPrefPlaylist::slotUpdate() {
    initialiseDirList();
    //Bundled songs stat tracking
    checkBox_library_scan->setChecked((bool)m_pconfig->getValueString(
            ConfigKey("[Library]","RescanOnStartup")).toInt());
    checkbox_ID3_sync->setChecked((bool)m_pconfig->getValueString(
            ConfigKey("[Library]","WriteAudioTags")).toInt());
    checkBox_use_relative_path->setChecked((bool)m_pconfig->getValueString(
            ConfigKey("[Library]","UseRelativePathOnExport")).toInt());
    checkBox_show_rhythmbox->setChecked((bool)m_pconfig->getValueString(
            ConfigKey("[Library]","ShowRhythmboxLibrary"),"1").toInt());
    checkBox_show_banshee->setChecked((bool)m_pconfig->getValueString(
            ConfigKey("[Library]","ShowBansheeLibrary"),"1").toInt());
    checkBox_show_itunes->setChecked((bool)m_pconfig->getValueString(
            ConfigKey("[Library]","ShowITunesLibrary"),"1").toInt());
    checkBox_show_traktor->setChecked((bool)m_pconfig->getValueString(
            ConfigKey("[Library]","ShowTraktorLibrary"),"1").toInt());
}

void DlgPrefPlaylist::slotAddDir() {
    QString fd = QFileDialog::getExistingDirectory(
        this, tr("Choose a music library directory"),
        QDesktopServices::storageLocation(QDesktopServices::MusicLocation));
    if (!fd.isEmpty()) {
        emit(requestAddDir(fd));
        slotUpdate();
    }
}

void DlgPrefPlaylist::slotRemoveDir() {
    QModelIndex index = dirList->currentIndex();
    QString fd = index.data().toString();
    QMessageBox removeMsgBox;
    removeMsgBox.setText(tr("Mixxx will hide any information about tracks in "
                            "this directory. Once you re-add the directory all "
                            "metadata will be restored"));
    removeMsgBox.setInformativeText(tr("Do you want to remove this directory?"));
    removeMsgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    QPushButton *removeAllButton = removeMsgBox.addButton(tr("Remove Metadata"),
            QMessageBox::YesRole);
    removeMsgBox.setDefaultButton(QMessageBox::Cancel);

    int ret = removeMsgBox.exec();
    bool removeAll = removeMsgBox.clickedButton() == removeAllButton;

    if (ret == QMessageBox::Yes || removeAll) {
        emit(requestRemoveDir(fd, removeAll));
        slotUpdate();
    }

}

void DlgPrefPlaylist::slotRelocateDir() {
    QModelIndex index = dirList->currentIndex();
    QString currentFd = index.data().toString();

    // If the selected directory exists, use it. If not, go up one directory (if
    // that directory exists). If neither exist, use the default music
    // directory.
    QString startDir = currentFd;
    QDir dir(startDir);
    if (!dir.exists() && dir.cdUp()) {
        startDir = dir.absolutePath();
    } else if (!dir.exists()) {
        startDir = QDesktopServices::storageLocation(
            QDesktopServices::MusicLocation);
    }

    QString fd = QFileDialog::getExistingDirectory(
        this, tr("relocate to directory"), startDir);

    if (!fd.isEmpty()) {
        emit(requestRelocateDir(currentFd, fd));
        slotUpdate();
    }
}

void DlgPrefPlaylist::slotApply() {
    m_pconfig->set(ConfigKey("[Library]","RescanOnStartup"),
                ConfigValue((int)checkBox_library_scan->isChecked()));
    m_pconfig->set(ConfigKey("[Library]","WriteAudioTags"),
                ConfigValue((int)checkbox_ID3_sync->isChecked()));
    m_pconfig->set(ConfigKey("[Library]","UseRelativePathOnExport"),
                ConfigValue((int)checkBox_use_relative_path->isChecked()));
    m_pconfig->set(ConfigKey("[Library]","ShowRhythmboxLibrary"),
                ConfigValue((int)checkBox_show_rhythmbox->isChecked()));
    m_pconfig->set(ConfigKey("[Library]","ShowBansheeLibrary"),
                ConfigValue((int)checkBox_show_banshee->isChecked()));
    m_pconfig->set(ConfigKey("[Library]","ShowITunesLibrary"),
                ConfigValue((int)checkBox_show_itunes->isChecked()));
    m_pconfig->set(ConfigKey("[Library]","ShowTraktorLibrary"),
                ConfigValue((int)checkBox_show_traktor->isChecked()));

    m_pconfig->Save();
}
