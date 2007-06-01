/*
* This file is part of KDevelop
*
* Copyright (c) 2006 Adam Treat <treat@kde.org>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Library General Public License as
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

#ifndef KDEVPARSEJOB_H
#define KDEVPARSEJOB_H

#include <JobSequence.h>
#include <kurl.h>
#include <languageexport.h>


class QByteArray;

namespace KTextEditor
{
class SmartRange;
}

namespace KDevelop
{

class BackgroundParser;

/**
 * The base class for background parser jobs.
 */
class KDEVPLATFORMLANGUAGE_EXPORT ParseJob : public ThreadWeaver::JobSequence
{
    Q_OBJECT
public:
    ParseJob( const KUrl &url, QObject *parent );

    virtual ~ParseJob();

    BackgroundParser* backgroundParser() const;
    void setBackgroundParser(BackgroundParser* parser);

    void setActiveDocument();
    virtual int priority() const;

    /// Returns whether the editor can provide the contents of the document or not.
    bool contentsAvailableFromEditor() const;

    /// Retrieve the contents of the file from the currently open editor.
    /// Ensure it is loaded by calling editorLoaded() first.
    QString contentsFromEditor(bool saveRevisionToken = false);

    /// Returns the revision token issued by the document's smart interface,
    /// or -1 if there was a problem.
    int revisionToken() const;

    KUrl document() const;

    void setErrorMessage(const QString& message);
    QString errorMessage() const;

    /// Overriden to allow jobs to determine if they've been requested to abort
    virtual void requestAbort();
    /// Determine if the job has been requested to abort
    bool abortRequested() const;
    /// Sets success to false, causing failed() to be emitted
    void abortJob();

    /// Overridden to convey whether the job succeeded or not.
    virtual bool success() const;

    /// Overridden to set the DependencyPolicy on subjobs.
    virtual void addJob(Job* job);

    /**
     * Attempt to add \a dependency as a dependency of \a actualDependee, which must
     * be a subjob of this job, or null (in which case, the dependency is added
     * to this job).  If a circular dependency is detected, the dependency will
     * not be added and the method will return false.
     */
    bool addDependency(ParseJob* dependency, ThreadWeaver::Job* actualDependee = 0);

protected:
    KUrl m_document;
    QString m_errorMessage;
    BackgroundParser* m_backgroundParser;

    QMutex* m_abortMutex;
    volatile bool m_abortRequested : 1;
    bool m_aborted : 1;
    int m_revisionToken;
private:
    class ParseJobPrivate* const d;
};

}
#endif

// kate: space-indent on; indent-width 4; tab-width 4; replace-tabs on
