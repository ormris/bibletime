/*********
*
* In the name of the Father, and of the Son, and of the Holy Spirit.
*
* This file is part of BibleTime's source code, http://www.bibletime.info/.
*
* Copyright 1999-2014 by the BibleTime developers.
* The BibleTime source code is licensed under the GNU General Public License
* version 2.0.
*
**********/

#include "btmoduletextmodel.h"

#include "backend/drivers/cswordmoduleinfo.h"
#include "backend/drivers/cswordbiblemoduleinfo.h"
#include "backend/drivers/cswordbookmoduleinfo.h"
#include "backend/drivers/cswordlexiconmoduleinfo.h"
#include "backend/keys/cswordtreekey.h"
#include "backend/managers/cswordbackend.h"

BtModuleTextModel::BtModuleTextModel(QObject *parent)
    : QAbstractListModel(parent), m_firstEntry(0), m_maxEntries(0) {
    QHash<int, QByteArray> roleNames;
    roleNames[ModuleEntry::ReferenceRole] =  "ref";
    roleNames[ModuleEntry::TextRole] = "line";
    setRoleNames(roleNames);
    m_displayOptions.verseNumbers = 0;
    m_displayOptions.lineBreaks = 1;
    m_filterOptions.footnotes = 0;
    m_filterOptions.greekAccents = 1;
    m_filterOptions.headings = 1;
    m_filterOptions.hebrewCantillation = 1;
    m_filterOptions.hebrewPoints = 1;
    m_filterOptions.lemmas = 0;
    m_filterOptions.morphSegmentation = 1;
    m_filterOptions.morphTags = 1;
    m_filterOptions.redLetterWords = 1;
    m_filterOptions.scriptureReferences = 0;
    m_filterOptions.strongNumbers = 0;
    m_filterOptions.textualVariants = 1;
    m_filterOptions.textualVariants = 0;
}

void BtModuleTextModel::setModules(const QStringList& modules) {
    beginResetModel();

    m_moduleInfoList.clear();
    for (int i = 0; i < modules.count(); ++i) {
        QString moduleName = modules.at(i);
        CSwordModuleInfo* module = CSwordBackend::instance()->findModuleByName(moduleName);
        m_moduleInfoList.append(module);
    }

    const CSwordModuleInfo* firstModule = m_moduleInfoList.at(0);

    if (isBible() || isCommentary())
    {
        const CSwordBibleModuleInfo *bm = qobject_cast<const CSwordBibleModuleInfo*>(firstModule);
        m_firstEntry = bm->lowerBound().getIndex();
        m_maxEntries = bm->upperBound().getIndex() - m_firstEntry + 1;
    }

    else if(isLexicon())
    {
        const CSwordLexiconModuleInfo *lm = qobject_cast<const CSwordLexiconModuleInfo*>(firstModule);
        m_maxEntries = lm->entries().size();
    }

    else if(isBook())
    {
        const CSwordBookModuleInfo *bookModule = qobject_cast<const CSwordBookModuleInfo*>(firstModule);
        sword::TreeKeyIdx tk(*bookModule->tree());
        tk.root();
        tk.firstChild();
        Q_ASSERT(tk.getOffset() == 4);
        tk.setPosition(sword::BOTTOM);
        m_maxEntries = tk.getOffset() / 4;
    }

    endResetModel();
}

QVariant BtModuleTextModel::data(const QModelIndex & index, int role) const {

    if (isBible() || isCommentary())
        return verseData(index, role);
    else if(isBook())
        return bookData(index, role);
    return QVariant("invalid");
}

QVariant BtModuleTextModel::bookData(const QModelIndex & index, int role) const {
    if (role == ModuleEntry::TextRole) {
        const CSwordBookModuleInfo *bookModule = qobject_cast<const CSwordBookModuleInfo*>(m_moduleInfoList.at(0));
        CSwordTreeKey key(bookModule->tree(), bookModule);
        int bookIndex = index.row() * 4;
        key.setIndex(bookIndex);
        QString keyName = key.key();
        Rendering::CEntryDisplay ed;
        QString text = ed.text(QList<const CSwordModuleInfo*>() << bookModule, key.key(), m_displayOptions, m_filterOptions);
        return text;
    }
    return QString();
}

QVariant BtModuleTextModel::verseData(const QModelIndex & index, int role) const {
    CSwordVerseKey key = indexToVerseKey(index);
    int verse = key.getVerse();
    if (role == ModuleEntry::TextRole) {
        if (verse == 0)
            return QString();
        QString text;
        if (verse == 1)
            text += "<center><b><font size='+1'\">"
                    + key.book() + " " + QString::number(key.getChapter()) + "</font></b></center><br>";

        text += Rendering::CEntryDisplay().text(m_moduleInfoList, key.key(), m_displayOptions, m_filterOptions);
        return text;
    }
    return QString();
}

int BtModuleTextModel::columnCount(const QModelIndex & /*parent*/) const {
    return 1;
}

int BtModuleTextModel::rowCount(const QModelIndex & /*parent*/) const {
    return m_maxEntries;
}

QHash<int, QByteArray> BtModuleTextModel::roleNames() const {
    return m_roleNames;
}

void BtModuleTextModel::setRoleNames(const QHash<int, QByteArray> &roleNames) {
    m_roleNames = roleNames;
}

bool BtModuleTextModel::isBible() const {
    const CSwordModuleInfo* module = m_moduleInfoList.at(0);
    if (module == 0)
        return false;
    return module->type() == CSwordModuleInfo::Bible;
}

bool BtModuleTextModel::isBook() const {
    const CSwordModuleInfo* module = m_moduleInfoList.at(0);
    if (module == 0)
        return false;
    return module->type() == CSwordModuleInfo::GenericBook;
}

bool BtModuleTextModel::isCommentary() const {
    const CSwordModuleInfo* module = m_moduleInfoList.at(0);
    if (module == 0)
        return false;
    return module->type() == CSwordModuleInfo::Commentary;
}

bool BtModuleTextModel::isLexicon() const {
    const CSwordModuleInfo* module = m_moduleInfoList.at(0);
    if (module == 0)
        return false;
    return module->type() == CSwordModuleInfo::Lexicon;
}

int BtModuleTextModel::verseKeyToIndex(const CSwordVerseKey& key) const {
    int index = key.getIndex() - m_firstEntry;
    return index;
}

CSwordVerseKey BtModuleTextModel::indexToVerseKey(const QModelIndex &index) const
{
    const CSwordModuleInfo* module = m_moduleInfoList.at(0);
    CSwordVerseKey key(module);

    key.setIntros(true);
    key.setIndex(index.row() + m_firstEntry);
    return key;
}
