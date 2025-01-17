/* This file is part of KDevelop
   Copyright (C) 2008 Cédric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2011 David Nolden <david.nolden.kdevelop@art-master.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
 */

#include "customscript_plugin.h"

#include <KPluginFactory>
#include <QTextStream>
#include <QTemporaryFile>
#include <QMimeDatabase>
#include <KProcess>
#include <interfaces/icore.h>
#include <interfaces/isourceformattercontroller.h>
#include <interfaces/isourceformatter.h>
#include <memory>
#include <QDir>
#include <QTimer>

#include <util/formattinghelpers.h>
#include <interfaces/iprojectcontroller.h>
#include <interfaces/iproject.h>
#include <KMessageBox>
#include <interfaces/iuicontroller.h>
#include <KParts/MainWindow>
#include <KLocalizedString>
#include <interfaces/ilanguagecontroller.h>
#include <language/interfaces/ilanguagesupport.h>
#include <util/path.h>
#include <debug.h>

using namespace KDevelop;

static QPointer<CustomScriptPlugin> indentPluginSingleton;

K_PLUGIN_FACTORY_WITH_JSON(CustomScriptFactory, "kdevcustomscript.json", registerPlugin<CustomScriptPlugin>(); )

// Replaces ${KEY} in command with variables[KEY]
static QString replaceVariables(QString command, const QMap<QString, QString>& variables)
{
    while (command.contains(QLatin1String("${"))) {
        int pos = command.indexOf(QLatin1String("${"));
        int end = command.indexOf(QLatin1Char('}'), pos + 2);
        if (end == -1) {
            break;
        }
        QString key = command.mid(pos + 2, end - pos - 2);

        const auto variableIt = variables.constFind(key);
        if (variableIt != variables.constEnd()) {
            command.replace(pos, 1 + end - pos, *variableIt );
        } else {
            qCDebug(CUSTOMSCRIPT) << "found no variable while replacing in shell-command" << command << "key" << key << "available:" << variables;
            command.remove(pos, 1 + end - pos);
        }
    }
    return command;
}

CustomScriptPlugin::CustomScriptPlugin(QObject* parent, const QVariantList&)
    : IPlugin(QStringLiteral("kdevcustomscript"), parent)
{
    m_currentStyle = predefinedStyles().at(0);
    indentPluginSingleton = this;
}

CustomScriptPlugin::~CustomScriptPlugin()
{
}

QString CustomScriptPlugin::name() const
{
    // This needs to match the X-KDE-PluginInfo-Name entry from the .desktop file!
    return QStringLiteral("kdevcustomscript");
}

QString CustomScriptPlugin::caption() const
{
    return QStringLiteral("Custom Script Formatter");
}

QString CustomScriptPlugin::description() const
{
    return i18n("<b>Indent and Format Source Code.</b><br />"
                "This plugin allows using powerful external formatting tools "
                "that can be invoked through the command-line.<br />"
                "For example, the <b>uncrustify</b>, <b>astyle</b> or <b>indent</b> "
                "formatters can be used.<br />"
                "The advantage of command-line formatters is that formatting configurations "
                "can be easily shared by all team members, independent of their preferred IDE.");
}

QString CustomScriptPlugin::formatSourceWithStyle(SourceFormatterStyle style, const QString& text, const QUrl& url, const QMimeType& /*mime*/, const QString& leftContext, const QString& rightContext) const
{
    KProcess proc;
    QTextStream ios(&proc);

    std::unique_ptr<QTemporaryFile> tmpFile;

    if (style.content().isEmpty()) {
        style = predefinedStyle(style.name());
        if (style.content().isEmpty()) {
            qCWarning(CUSTOMSCRIPT) << "Empty contents for style" << style.name() << "for indent plugin";
            return text;
        }
    }

    QString useText = text;
    useText = leftContext + useText + rightContext;

    QMap<QString, QString> projectVariables;
    const auto projects = ICore::self()->projectController()->projects();
    for (IProject* project : projects) {
        projectVariables[project->name()] = project->path().toUrl().toLocalFile();
    }

    QString command = style.content();

    // Replace ${Project} with the project path
    command = replaceVariables(command, projectVariables);
    command.replace(QLatin1String("$FILE"), url.toLocalFile());

    if (command.contains(QLatin1String("$TMPFILE"))) {
        tmpFile.reset(new QTemporaryFile(QDir::tempPath() + QLatin1String("/code")));
        tmpFile->setAutoRemove(false);
        if (tmpFile->open()) {
            qCDebug(CUSTOMSCRIPT) << "using temporary file" << tmpFile->fileName();
            command.replace(QLatin1String("$TMPFILE"), tmpFile->fileName());
            QByteArray useTextArray = useText.toLocal8Bit();
            if (tmpFile->write(useTextArray) != useTextArray.size()) {
                qCWarning(CUSTOMSCRIPT) << "failed to write text to temporary file";
                return text;
            }
        } else {
            qCWarning(CUSTOMSCRIPT) << "Failed to create a temporary file";
            return text;
        }
        tmpFile->close();
    }

    qCDebug(CUSTOMSCRIPT) << "using shell command for indentation: " << command;
    proc.setShellCommand(command);
    proc.setOutputChannelMode(KProcess::OnlyStdoutChannel);

    proc.start();
    if (!proc.waitForStarted()) {
        qCDebug(CUSTOMSCRIPT) << "Unable to start indent" << endl;
        return text;
    }

    if (!tmpFile.get()) {
        proc.write(useText.toLocal8Bit());
    }

    proc.closeWriteChannel();
    if (!proc.waitForFinished()) {
        qCDebug(CUSTOMSCRIPT) << "Process doesn't finish" << endl;
        return text;
    }

    QString output;

    if (tmpFile.get()) {
        QFile f(tmpFile->fileName());
        if (f.open(QIODevice::ReadOnly)) {
            output = QString::fromLocal8Bit(f.readAll());
        } else {
            qCWarning(CUSTOMSCRIPT) << "Failed opening the temporary file for reading";
            return text;
        }
    } else {
        output = ios.readAll();
    }
    if (output.isEmpty()) {
        qCWarning(CUSTOMSCRIPT) << "indent returned empty text for style" << style.name() << style.content();
        return text;
    }

    int tabWidth = 4;
    if ((!leftContext.isEmpty() || !rightContext.isEmpty()) && (text.contains(QLatin1Char('	')) || output.contains(QLatin1Char('\t')))) {
        // If we have to do contex-matching with tabs, determine the correct tab-width so that the context
        // can be matched correctly
        Indentation indent = indentation(url);
        if (indent.indentationTabWidth > 0) {
            tabWidth = indent.indentationTabWidth;
        }
    }

    return KDevelop::extractFormattedTextFromContext(output, text, leftContext, rightContext, tabWidth);
}

QString CustomScriptPlugin::formatSource(const QString& text, const QUrl& url, const QMimeType& mime, const QString& leftContext, const QString& rightContext) const
{
	auto style = KDevelop::ICore::self()->sourceFormatterController()->styleForUrl(url, mime);
    return formatSourceWithStyle(style, text, url, mime, leftContext, rightContext);
}

static QVector<SourceFormatterStyle> stylesFromLanguagePlugins()
{
    QVector<KDevelop::SourceFormatterStyle> styles;
    const auto loadedLanguages = ICore::self()->languageController()->loadedLanguages();
    for (auto* lang : loadedLanguages) {
        const SourceFormatterItemList& languageStyles = lang->sourceFormatterItems();
        for (const SourceFormatterStyleItem& item: languageStyles) {
            if (item.engine == QLatin1String("customscript")) {
                styles << item.style;
            }
        }
    }

    return styles;
}

KDevelop::SourceFormatterStyle CustomScriptPlugin::predefinedStyle(const QString& name) const
{
	const auto& langStyles = stylesFromLanguagePlugins();
    for (auto& langStyle: langStyles) {
        qCDebug(CUSTOMSCRIPT) << "looking at style from language with custom sample" << langStyle.description() << langStyle.overrideSample();
        if (langStyle.name() == name) {
            return langStyle;
        }
    }

    SourceFormatterStyle result(name);
    if (name == QLatin1String("GNU_indent_GNU")) {
        result.setCaption(i18n("Gnu Indent: GNU"));
        result.setContent(QStringLiteral("indent"));
        result.setUsePreview(true);
    } else if (name == QLatin1String("GNU_indent_KR")) {
        result.setCaption(i18n("Gnu Indent: Kernighan & Ritchie"));
        result.setContent(QStringLiteral("indent -kr"));
        result.setUsePreview(true);
    } else if (name == QLatin1String("GNU_indent_orig")) {
        result.setCaption(i18n("Gnu Indent: Original Berkeley indent style"));
        result.setContent(QStringLiteral("indent -orig"));
        result.setUsePreview(true);
    } else if (name == QLatin1String("kdev_format_source")) {
        result.setCaption(QStringLiteral("KDevelop: kdev_format_source"));
        result.setContent(QStringLiteral("kdev_format_source $FILE $TMPFILE"));
        result.setUsePreview(false);
        result.setDescription(i18n("Description:<br />"
                                   "<b>kdev_format_source</b> is a script bundled with KDevelop "
                                   "which allows using fine-grained formatting rules by placing "
                                   "meta-files called <b>format_sources</b> into the file-system.<br /><br />"
                                   "Each line of the <b>format_sources</b> files defines a list of wildcards "
                                   "followed by a colon and the used formatting-command.<br /><br />"
                                   "The formatting-command should use <b>$TMPFILE</b> to reference the "
                                   "temporary file to reformat.<br /><br />"
                                   "Example:<br />"
                                   "<b>*.cpp *.h : myformatter $TMPFILE</b><br />"
                                   "This will reformat all files ending with <b>.cpp</b> or <b>.h</b> using "
                                   "the custom formatting script <b>myformatter</b>.<br /><br />"
                                   "Example: <br />"
                                   "<b>subdir/* : uncrustify -l CPP -f $TMPFILE -c uncrustify.config -o $TMPFILE</b> <br />"
                                   "This will reformat all files in subdirectory <b>subdir</b> using the <b>uncrustify</b> "
                                   "tool with the config-file <b>uncrustify.config</b>."));
    }
    result.setMimeTypes({
        {QStringLiteral("text/x-c++src"), QStringLiteral("C++")},
        {QStringLiteral("text/x-chdr"),   QStringLiteral("C")},
        {QStringLiteral("text/x-c++hdr"), QStringLiteral("C++")},
        {QStringLiteral("text/x-csrc"),   QStringLiteral("C")},
        {QStringLiteral("text/x-java"),   QStringLiteral("Java")},
        {QStringLiteral("text/x-csharp"), QStringLiteral("C#")},
        {QStringLiteral("text/x-objcsrc"), QStringLiteral("Objective-C")},
        {QStringLiteral("text/x-objc++src"), QStringLiteral("Objective-C++")},
        {QStringLiteral("text/x-objchdr"), QStringLiteral("Objective-C")},
    });
    return result;
}

QVector<KDevelop::SourceFormatterStyle> CustomScriptPlugin::predefinedStyles() const
{
    const QVector<KDevelop::SourceFormatterStyle> styles = stylesFromLanguagePlugins() + QVector<KDevelop::SourceFormatterStyle>{
        predefinedStyle(QStringLiteral("kdev_format_source")),
        predefinedStyle(QStringLiteral("GNU_indent_GNU")),
        predefinedStyle(QStringLiteral("GNU_indent_KR")),
        predefinedStyle(QStringLiteral("GNU_indent_orig")),
    };
    return styles;
}

KDevelop::SettingsWidget* CustomScriptPlugin::editStyleWidget(const QMimeType& mime) const
{
    Q_UNUSED(mime);
    return new CustomScriptPreferences();
}

static QString formattingSample()
{
    return QStringLiteral(
        "// Formatting\n"
        "void func(){\n"
        "\tif(isFoo(a,b))\n"
        "\tbar(a,b);\n"
        "if(isFoo)\n"
        "\ta=bar((b-c)*a,*d--);\n"
        "if(  isFoo( a,b ) )\n"
        "\tbar(a, b);\n"
        "if (isFoo) {isFoo=false;cat << isFoo <<endl;}\n"
        "if(isFoo)DoBar();if (isFoo){\n"
        "\tbar();\n"
        "}\n"
        "\telse if(isBar()){\n"
        "\tannotherBar();\n"
        "}\n"
        "int var = 1;\n"
        "int *ptr = &var;\n"
        "int &ref = i;\n"
        "\n"
        "QList<int>::const_iterator it = list.begin();\n"
        "}\n"
        "namespace A {\n"
        "namespace B {\n"
        "void foo() {\n"
        "  if (true) {\n"
        "    func();\n"
        "  } else {\n"
        "    // bla\n"
        "  }\n"
        "}\n"
        "}\n"
        "}\n");
}

static QString indentingSample()
{
    return QStringLiteral(
        "// Indentation\n"
        "#define foobar(A)\\\n"
        "{Foo();Bar();}\n"
        "#define anotherFoo(B)\\\n"
        "return Bar()\n"
        "\n"
        "namespace Bar\n"
        "{\n"
        "class Foo\n"
        "{public:\n"
        "Foo();\n"
        "virtual ~Foo();\n"
        "};\n"
        "void bar(int foo)\n"
        "{\n"
        "switch (foo)\n"
        "{\n"
        "case 1:\n"
        "a+=1;\n"
        "break;\n"
        "case 2:\n"
        "{\n"
        "a += 2;\n"
        " break;\n"
        "}\n"
        "}\n"
        "if (isFoo)\n"
        "{\n"
        "bar();\n"
        "}\n"
        "else\n"
        "{\n"
        "anotherBar();\n"
        "}\n"
        "}\n"
        "int foo()\n"
        "\twhile(isFoo)\n"
        "\t\t{\n"
        "\t\t\t// ...\n"
        "\t\t\tgoto error;\n"
        "\t\t/* .... */\n"
        "\t\terror:\n"
        "\t\t\t//...\n"
        "\t\t}\n"
        "\t}\n"
        "fooArray[]={ red,\n"
        "\tgreen,\n"
        "\tdarkblue};\n"
        "fooFunction(barArg1,\n"
        "\tbarArg2,\n"
        "\tbarArg3);\n"
        );
}

QString CustomScriptPlugin::previewText(const SourceFormatterStyle& style, const QMimeType& /*mime*/) const
{
    if (!style.overrideSample().isEmpty()) {
        return style.overrideSample();
    }
    return formattingSample() + QLatin1String("\n\n") + indentingSample();
}

QStringList CustomScriptPlugin::computeIndentationFromSample(const QUrl& url) const
{
    QStringList ret;

    auto languages = ICore::self()->languageController()->languagesForUrl(url);
    if (languages.isEmpty()) {
        return ret;
    }

    QString sample = languages[0]->indentationSample();
    QString formattedSample = formatSource(sample, url, QMimeDatabase().mimeTypeForUrl(url), QString(), QString());

    const QStringList lines = formattedSample.split(QLatin1Char('\n'));
    for (const QString& line : lines) {
        if (!line.isEmpty() && line[0].isSpace()) {
            QString indent;
            for (const QChar c : line) {
                if (c.isSpace()) {
                    indent.push_back(c);
                } else {
                    break;
                }
            }

            if (!indent.isEmpty() && !ret.contains(indent)) {
                ret.push_back(indent);
            }
        }
    }

    return ret;
}

CustomScriptPlugin::Indentation CustomScriptPlugin::indentation(const QUrl& url) const
{
    Indentation ret;
    QStringList indent = computeIndentationFromSample(url);
    if (indent.isEmpty()) {
        qCDebug(CUSTOMSCRIPT) << "failed extracting a valid indentation from sample for url" << url;
        return ret; // No valid indentation could be extracted
    }

    if (indent[0].contains(QLatin1Char(' '))) {
        ret.indentWidth = indent[0].count(QLatin1Char(' '));
    }

    if (!indent.join(QString()).contains(QLatin1Char('	'))) {
        ret.indentationTabWidth = -1;         // Tabs are not used for indentation
    }
    if (indent[0] == QLatin1String("	")) {
        // The script indents with tabs-only
        // The problem is that we don't know how
        // wide a tab is supposed to be.
        //
        // We need indentation-width=tab-width
        // to make the editor do tab-only formatting,
        // so choose a random with of 4.
        ret.indentWidth = 4;
        ret.indentationTabWidth = 4;
    } else if (ret.indentWidth)   {
        // Tabs are used for indentation, alongside with spaces
        // Try finding out how many spaces one tab stands for.
        // Do it by assuming a uniform indentation-step with each level.

        for (int pos = 0; pos < indent.size(); ++pos) {
            if (indent[pos] == QLatin1String("	")&& pos >= 1) {
                // This line consists of only a tab.
                int prevWidth = indent[pos - 1].length();
                int prevPrevWidth = (pos >= 2) ? indent[pos - 2].length() : 0;
                int step = prevWidth - prevPrevWidth;
                qCDebug(CUSTOMSCRIPT) << "found in line " << pos << prevWidth << prevPrevWidth << step;
                if (step > 0 && step <= prevWidth) {
                    qCDebug(CUSTOMSCRIPT) << "Done";
                    ret.indentationTabWidth = prevWidth + step;
                    break;
                }
            }
        }
    }

    qCDebug(CUSTOMSCRIPT) << "indent-sample" << QLatin1Char('\"') + indent.join(QLatin1Char('\n')) + QLatin1Char('\"') << "extracted tab-width" << ret.indentationTabWidth << "extracted indentation width" << ret.indentWidth;

    return ret;
}

void CustomScriptPreferences::updateTimeout()
{
    const QString& text = indentPluginSingleton.data()->previewText(m_style, QMimeType());
    QString formatted = indentPluginSingleton.data()->formatSourceWithStyle(m_style, text, QUrl(), QMimeType());
    emit previewTextChanged(formatted);
}

CustomScriptPreferences::CustomScriptPreferences()
{
    m_updateTimer = new QTimer(this);
    m_updateTimer->setSingleShot(true);
    m_updateTimer->setInterval(1000);
    connect(m_updateTimer, &QTimer::timeout, this, &CustomScriptPreferences::updateTimeout);
    m_vLayout = new QVBoxLayout(this);
    m_vLayout->setMargin(0);
    m_captionLabel = new QLabel;
    m_vLayout->addWidget(m_captionLabel);
    m_vLayout->addSpacing(10);
    m_hLayout = new QHBoxLayout;
    m_vLayout->addLayout(m_hLayout);
    m_commandLabel = new QLabel;
    m_hLayout->addWidget(m_commandLabel);
    m_commandEdit = new QLineEdit;
    m_hLayout->addWidget(m_commandEdit);
    m_commandLabel->setText(i18n("Command:"));
    m_vLayout->addSpacing(10);
    m_bottomLabel = new QLabel;
    m_vLayout->addWidget(m_bottomLabel);
    m_bottomLabel->setTextFormat(Qt::RichText);
    m_bottomLabel->setText(
        i18n("<i>You can enter an arbitrary shell command.</i><br />"
             "The unformatted source-code is reached to the command <br />"
             "through the standard input, and the <br />"
             "formatted result is read from the standard output.<br />"
             "<br />"
             "If you add <b>$TMPFILE</b> into the command, then <br />"
             "a temporary file is used for transferring the code."));
    connect(m_commandEdit, &QLineEdit::textEdited,
            m_updateTimer, QOverload<>::of(&QTimer::start));

    m_vLayout->addSpacing(10);

    m_moreVariablesButton = new QPushButton(i18n("More Variables"));
    connect(m_moreVariablesButton, &QPushButton::clicked, this, &CustomScriptPreferences::moreVariablesClicked);
    m_vLayout->addWidget(m_moreVariablesButton);
    m_vLayout->addStretch();
}

void CustomScriptPreferences::load(const KDevelop::SourceFormatterStyle& style)
{
    m_style = style;
    m_commandEdit->setText(style.content());
    m_captionLabel->setText(i18n("Style: %1", style.caption()));

    updateTimeout();
}

QString CustomScriptPreferences::save()
{
    return m_commandEdit->text();
}

void CustomScriptPreferences::moreVariablesClicked(bool)
{
    KMessageBox::information(ICore::self()->uiController()->activeMainWindow(),
                             i18n("<b>$TMPFILE</b> will be replaced with the path to a temporary file. <br />"
                                  "The code will be written into the file, the temporary <br />"
                                  "file will be substituted into that position, and the result <br />"
                                  "will be read out of that file. <br />"
                                  "<br />"
                                  "<b>$FILE</b> will be replaced with the path of the original file. <br />"
                                  "The contents of the file must not be modified, changes are allowed <br />"
                                  "only in $TMPFILE.<br />"
                                  "<br />"
                                  "<b>${PROJECT_NAME}</b> will be replaced by the path of <br />"
                                  "the currently open project with the matching name."

                                  ), i18n("Variable Replacements"));
}

#include "customscript_plugin.moc"
