/***************************************************************************
 *   Copyright (C) 2001 by Bernd Gehrmann                                  *
 *   bernd@kdevelop.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qdom.h>
#include <qlayout.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <kbuttonbox.h>
#include <kdebug.h>
#include <kdialog.h>
#include <klibloader.h>
#include <klocale.h>
#include <kseparator.h>
#include <kservice.h>
#include <ktrader.h>

#include "domutil.h"
#include "servicecombobox.h"
#include "autoprojectpart.h"
#include "projectoptionsdlg.h"


ProjectOptionsDialog::ProjectOptionsDialog(AutoProjectPart *part, QWidget *parent, const char *name)
    : QTabDialog(parent, name, true)
{
    setCaption(i18n("Project options"));
    
    m_part = part;

    addTab(createCompilerTab(), i18n("Compiler"));
    //    addTab(createLinkerTab(), i18n("Linker"));
    addTab(createMakeTab(), i18n("Make"));

    setCancelButton();
    connect( this, SIGNAL(applyButtonPressed()), this, SLOT(accept()) );
    connect( this, SIGNAL(cancelButtonPressed()), this, SLOT(reject()) );
    
    init();
}


QWidget *ProjectOptionsDialog::createCompilerTab()
{
    QWidget *w = new QWidget(this, "compiler tab");
    
    QLabel *ccompiler_label   = new QLabel(i18n("C Compiler"), w);
    QLabel *cxxcompiler_label = new QLabel(i18n("C++ Compiler"), w);
    QLabel *f77compiler_label = new QLabel(i18n("Fortran Compiler"), w);
    QLabel *cbinary_label     = new QLabel(i18n("Compiler command (CC):"), w);
    QLabel *cxxbinary_label   = new QLabel(i18n("Compiler command (CXX):"), w);
    QLabel *f77binary_label   = new QLabel(i18n("Compiler command (F77):"), w);

    KTrader::OfferList coffers =
        KTrader::self()->query("KDevelop/CompilerOptions", "[X-KDevelop-Language] == 'C'");
    KTrader::OfferList cxxoffers =
        KTrader::self()->query("KDevelop/CompilerOptions", "[X-KDevelop-Language] == 'C++'");
    KTrader::OfferList f77offers =
        KTrader::self()->query("KDevelop/CompilerOptions", "[X-KDevelop-Language] == 'Fortran'");
    
    cservice_combo   = new ServiceComboBox(coffers, w, "c services");
    cxxservice_combo = new ServiceComboBox(cxxoffers, w, "cpp services");
    f77service_combo = new ServiceComboBox(f77offers, w, "f77 services");

    cbinary_edit   = new QLineEdit(w, "cbinary");
    cxxbinary_edit = new QLineEdit(w, "cxxbinary");
    f77binary_edit = new QLineEdit(w, "f77binary");
    
    QHBox *cflags_box   = new QHBox(w);
    cflags_box->setSpacing(10);
    QHBox *cxxflags_box = new QHBox(w);
    cxxflags_box->setSpacing(10);
    QHBox *f77flags_box = new QHBox(w);
    f77flags_box->setSpacing(10);
    
    /*QLabel *cflags_label      =*/ new QLabel(i18n("Compiler flags (CFLAGS):"), cflags_box);
    /*QLabel *cxxflags_label    =*/ new QLabel(i18n("Compiler flags (CXXFLAGS):"), cxxflags_box);
    /*QLabel *f77flags_label    =*/ new QLabel(i18n("Compiler flags (FFLAGS):"), f77flags_box);

    cflags_edit   = new QLineEdit(cflags_box, "cflags");
    cxxflags_edit = new QLineEdit(cxxflags_box, "cxxflags");
    f77flags_edit = new QLineEdit(f77flags_box, "f77flags");

    QFontMetrics fm(cflags_edit->fontMetrics());
    int wid = fm.width('X')*35;
    cflags_edit->setMinimumWidth(wid);
    cxxflags_edit->setMinimumWidth(wid);
    f77flags_edit->setMinimumWidth(wid);
    
    QPushButton *cflags_button   = new QPushButton("...", cflags_box);
    cflags_button->setFixedSize(30, 25);
    QPushButton *cxxflags_button = new QPushButton("...", cxxflags_box);
    cxxflags_button->setFixedSize(30, 25);
    QPushButton *f77flags_button = new QPushButton("...", f77flags_box);
    f77flags_button->setFixedSize(30, 25);

    if (coffers.isEmpty())
        cflags_button->setEnabled(false);
    if (cxxoffers.isEmpty())
        cxxflags_button->setEnabled(false);
    if (f77offers.isEmpty())
        f77flags_button->setEnabled(false);
    
    connect(cflags_button,   SIGNAL(clicked()), this, SLOT(cflagsClicked()));
    connect(cxxflags_button, SIGNAL(clicked()), this, SLOT(cxxflagsClicked()));
    connect(f77flags_button, SIGNAL(clicked()), this, SLOT(f77flagsClicked()));

    QGridLayout *grid = new QGridLayout(w, 11, 3,
                                        2*KDialog::marginHint(), KDialog::spacingHint());
    grid->setColStretch(0, 1);
    grid->setColStretch(1, 2);
    grid->setColStretch(2, 1);
    grid->setRowStretch(0, 0);
    grid->setRowStretch(1, 0);
    grid->setRowStretch(2, 0);
    grid->setRowStretch(4, 0);
    grid->setRowStretch(5, 0);
    grid->setRowStretch(6, 0);
    grid->setRowStretch(8, 0);
    grid->setRowStretch(9, 0);
    grid->setRowStretch(10, 0);
    grid->setRowStretch(11, 0);
    grid->setRowStretch(12, 0);

    grid->addWidget(ccompiler_label, 0, 0);
    grid->addWidget(cbinary_label, 0, 1);
    grid->addWidget(cservice_combo, 1, 0);
    grid->addMultiCellWidget(cbinary_edit, 1, 1, 1, 2);
    grid->addMultiCellWidget(cflags_box, 2, 2, 0, 2);
    grid->addMultiCellWidget(new KSeparator(w), 3, 3, 0, 2);
    
    grid->addWidget(cxxcompiler_label, 4, 0);
    grid->addWidget(cxxbinary_label, 4, 1);
    grid->addWidget(cxxservice_combo, 5, 0);
    grid->addMultiCellWidget(cxxbinary_edit, 5, 5, 1, 2);
    grid->addMultiCellWidget(cxxflags_box, 6, 6, 0, 2);
    grid->addMultiCellWidget(new KSeparator(w), 7, 7, 0, 2);

    grid->addWidget(f77compiler_label, 8, 0);
    grid->addWidget(f77binary_label, 8, 1);
    grid->addWidget(f77service_combo, 9, 0);
    grid->addMultiCellWidget(f77binary_edit, 9, 9, 1, 2);
    grid->addMultiCellWidget(f77flags_box, 10, 10, 0, 2);

    setTabOrder(cservice_combo, cbinary_edit);
    setTabOrder(cbinary_edit, cflags_edit);
    setTabOrder(cflags_edit, cflags_button);
    setTabOrder(cflags_button, cxxservice_combo);
    setTabOrder(cxxservice_combo, cxxbinary_edit);
    setTabOrder(cxxbinary_edit, cxxflags_edit);
    setTabOrder(cxxflags_edit, cxxflags_button);
    setTabOrder(cxxflags_button, f77service_combo);
    setTabOrder(f77service_combo, f77binary_edit);
    setTabOrder(f77binary_edit, f77flags_edit);
    setTabOrder(f77flags_edit, f77flags_button);

    return w;
}


QWidget *ProjectOptionsDialog::createMakeTab()
{
    QWidget *w = new QWidget(this, "make tab");

    abort_box = new QCheckBox(i18n("Abort on first error"), w);

    QLabel *jobs_label = new QLabel(i18n("Number of simulaneous jobs:"), w);
    jobs_box = new QSpinBox(1, 10, 1, w);

    dontact_box = new QCheckBox(i18n("Display only commands without executing them"), w);

    QGridLayout *grid = new QGridLayout(w, 4, 2,
                                        2*KDialog::marginHint(), KDialog::spacingHint());
    grid->setRowStretch(3, 4);
    grid->addMultiCellWidget(abort_box, 0, 0, 0, 1);
    grid->addWidget(jobs_label, 1, 0);
    grid->addWidget(jobs_box, 1, 1);
    grid->addMultiCellWidget(dontact_box, 2, 2, 0, 1);
    
    return w;
}


ProjectOptionsDialog::~ProjectOptionsDialog()
{}


void ProjectOptionsDialog::cflagsClicked()
{
    KDevCompilerOptions *plugin = createCompilerOptions(cservice_combo->currentText());

    if (plugin) {
        QString flags = plugin->exec(this, cflags_edit->text());
        if (!flags.isNull())
            cflags_edit->setText(flags);
        delete plugin;
    }
}


void ProjectOptionsDialog::cxxflagsClicked()
{
    KDevCompilerOptions *plugin = createCompilerOptions(cxxservice_combo->currentText());

    if (plugin) {
        QString flags = plugin->exec(this, cxxflags_edit->text());
        if (!flags.isNull())
            cxxflags_edit->setText(flags);
        delete plugin;
    }
}


void ProjectOptionsDialog::f77flagsClicked()
{
    KDevCompilerOptions *plugin = createCompilerOptions(f77service_combo->currentText());

    if (plugin) {
        QString flags = plugin->exec(this, f77flags_edit->text());
        if (!flags.isNull())
            f77flags_edit->setText(flags);
        delete plugin;
    }
}


KDevCompilerOptions *ProjectOptionsDialog::createCompilerOptions(const QString &name)
{
    KService::Ptr service = KService::serviceByName(name);
    if (!service) {
        kdDebug(9000) << "Can't find service " << name;
        return 0;
    }
    
    KLibFactory *factory = KLibLoader::self()->factory(service->library());

    QStringList args;
    QVariant prop = service->property("X-KDevelop-Args");
    if (prop.isValid())
        args = QStringList::split(" ", prop.toString());
    
    QObject *obj = factory->create(this, service->name().latin1(),
                                   "KDevCompilerOptions", args);

    if (!obj->inherits("KDevCompilerOptions")) {
        kdDebug(9000) << "Component does not inherit KDevCompilerOptions" << endl;
        return 0;
    }
    KDevCompilerOptions *dlg = (KDevCompilerOptions*) obj;
    
    return dlg;
}


void ProjectOptionsDialog::init()
{
    QDomDocument doc = *m_part->document();

    //    kdDebug(9000) << "init";
    
    cservice_combo->setCurrentText(DomUtil::readEntry(doc, "/kdevautoproject/compiler/ccompiler"));
    cxxservice_combo->setCurrentText(DomUtil::readEntry(doc, "/kdevautoproject/compiler/cxxcompiler"));
    f77service_combo->setCurrentText(DomUtil::readEntry(doc, "/kdevautoproject/compiler/f77compiler"));

    //    kdDebug(9000) << "binary";
    cbinary_edit->setText(DomUtil::readEntry(doc, "/kdevautoproject/compiler/ccompilerbinary"));
    cxxbinary_edit->setText(DomUtil::readEntry(doc, "/kdevautoproject/compiler/cxxcompilerbinary"));
    f77binary_edit->setText(DomUtil::readEntry(doc, "/kdevautoproject/compiler/f77compilerbinary"));
    
    //    kdDebug(9000) << "flags";
    cflags_edit->setText(DomUtil::readEntry(doc, "/kdevautoproject/compiler/cflags"));
    cxxflags_edit->setText(DomUtil::readEntry(doc, "/kdevautoproject/compiler/cxxflags"));
    f77flags_edit->setText(DomUtil::readEntry(doc, "/kdevautoproject/compiler/f77flags"));

    kdDebug(9000) << "make";
    abort_box->setChecked(DomUtil::readBoolEntry(doc, "/kdevautoproject/make/abortonerror"));
    jobs_box->setValue(DomUtil::readIntEntry(doc, "/kdevautoproject/make/numberofjobs"));
    dontact_box->setChecked(DomUtil::readBoolEntry(doc, "/kdevautoproject/make/dontact"));
}


void ProjectOptionsDialog::accept()
{
    QDomDocument doc = *m_part->document();

    DomUtil::writeEntry(doc, "/kdevautoproject/compiler/ccompiler", cservice_combo->currentText());
    DomUtil::writeEntry(doc, "/kdevautoproject/compiler/cxxcompiler", cxxservice_combo->currentText());
    DomUtil::writeEntry(doc, "/kdevautoproject/compiler/f77compiler", f77service_combo->currentText());

    DomUtil::writeEntry(doc, "/kdevautoproject/compiler/ccompilerbinary", cbinary_edit->text());
    DomUtil::writeEntry(doc, "/kdevautoproject/compiler/cxxcompilerbinary", cxxbinary_edit->text());
    DomUtil::writeEntry(doc, "/kdevautoproject/compiler/f77compilerbinary", f77binary_edit->text());

    DomUtil::writeEntry(doc, "/kdevautoproject/compiler/cflags", cflags_edit->text());
    DomUtil::writeEntry(doc, "/kdevautoproject/compiler/cxxflags", cxxflags_edit->text());
    DomUtil::writeEntry(doc, "/kdevautoproject/compiler/f77flags", f77flags_edit->text());

    DomUtil::writeBoolEntry(doc, "/kdevautoproject/make/abortonerror", abort_box->isChecked());
    DomUtil::writeIntEntry(doc, "/kdevautoproject/make/numberofjobs", jobs_box->value());
    DomUtil::writeBoolEntry(doc, "/kdevautoproject/make/dontact", dontact_box->isChecked());

    QTabDialog::accept();
}
#include "projectoptionsdlg.moc"
