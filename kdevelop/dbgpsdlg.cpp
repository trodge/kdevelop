/***************************************************************************
                      dbgpsdlg.cpp  -  pid selector using ps
                             -------------------
    begin                : Mon Sep 20 1999
    copyright            : (C) 1999 by John Birch
    email                : jb.nz@writeme.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "dbgpsdlg.h"

//#include <kapp.h>
#include <kbuttonbox.h>
#include <kdialog.h>
#include <kglobalsettings.h>
#include <klocale.h>
#include <kprocess.h>

#include <qframe.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlistbox.h>
#include <qpushbutton.h>
#include <qtoolbutton.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

/***************************************************************************/

// Display a list of processes for the user to select one
// only display processes that they can do something with so if the user
// is root then display all processes
// For use with the internal debugger, but this dialog doesn't know anything
// about why it's doing it.

Dbg_PS_Dialog::Dbg_PS_Dialog(QWidget *parent, const char *name) :
  KDialog(parent, name, true),      // modal
  psProc_(0),
  pids_(new QListBox(this)),
  heading_(new QLabel(" ", this)),
  pidLines_(QString())
{
  setCaption(i18n("Attach to Process"));

  QBoxLayout *topLayout = new QVBoxLayout(this, 5);

  heading_->setFont(KGlobalSettings::fixedFont());
  heading_->setFrameStyle(QFrame::Panel|QFrame::Sunken);
  heading_->setMaximumHeight(heading_->sizeHint().height());
  heading_->setMinimumSize(heading_->sizeHint());
  topLayout->addWidget(heading_, 5);

  topLayout->addWidget(pids_, 5);
  pids_->setFont(KGlobalSettings::fixedFont());

  KButtonBox *buttonbox = new KButtonBox(this, Qt::Horizontal, 5);
  QPushButton *ok       = buttonbox->addButton(i18n("OK"));
  buttonbox->addStretch();
  QPushButton *cancel   = buttonbox->addButton(i18n("Cancel"));
  buttonbox->layout();
  topLayout->addWidget(buttonbox);

  connect(ok,     SIGNAL(clicked()),  SLOT(accept()));
  connect(cancel, SIGNAL(clicked()),  SLOT(reject()));

  psProc_ = new KShellProcess("/bin/sh");
  *psProc_ << "ps";
  *psProc_ << "x";
  pidCmd_ = "ps x";

  if (getuid() == 0)
  {
    *psProc_ << "a";
	  pidCmd_ += " a";
  }

  connect( psProc_, SIGNAL(processExited(KProcess *)),                SLOT(slotProcessExited()) );
  connect( psProc_, SIGNAL(receivedStdout(KProcess *, char *, int)),  SLOT(slotReceivedOutput(KProcess *, char *, int)) );
  psProc_->start(KProcess::NotifyOnExit, KProcess::Stdout);

  // Default display to 40 chars wide, default height is okay
  resize( ((KGlobalSettings::fixedFont()).pointSize())*40, height());
  topLayout->activate();
}

/***************************************************************************/

Dbg_PS_Dialog::~Dbg_PS_Dialog()
{
  if (psProc_)
    delete psProc_;
}

/***************************************************************************/

int Dbg_PS_Dialog::pidSelected()
{
  QString pidText = QString(pids_->text(pids_->currentItem()));
  if (pidText)
    return atoi(pidText);

  return 0;
}

/***************************************************************************/

void Dbg_PS_Dialog::slotReceivedOutput(KProcess */*proc*/, char *buffer, int buflen)
{
  pidLines_ += QString::fromLocal8Bit(buffer, buflen);
}

/***************************************************************************/

void Dbg_PS_Dialog::slotProcessExited()
{
  if (psProc_)
    delete psProc_;
  psProc_ = 0;

  pidLines_ += '\n';

  int start = pidLines_.find('\n', 0);  // Skip the first line (header line)
  int pos;
  if (start != -1)
    heading_->setText(pidLines_.left(start));
  while ( (pos = pidLines_.find('\n', start)) != -1)
  {
    QString item = pidLines_.mid(start, pos-start);
    if (!item.isEmpty())
    {
      if (item.find(pidCmd_) == -1)
        pids_->insertItem(item);
    }

    start = pos+1;
  }
}

/***************************************************************************/
#include "dbgpsdlg.moc"
