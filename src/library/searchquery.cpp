#include <QtDebug>

#include "library/searchquery.h"

#include "library/queryutil.h"
#include "track/keyutils.h"
#include "library/dao/trackdao.h"

QVariant getTrackValueForColumn(const TrackPointer& pTrack, const QString& column) {
    if (column == LIBRARYTABLE_ARTIST) {
        return pTrack->getArtist();
    } else if (column == LIBRARYTABLE_TITLE) {
        return pTrack->getTitle();
    } else if (column == LIBRARYTABLE_ALBUM) {
        return pTrack->getAlbum();
    } else if (column == LIBRARYTABLE_ALBUMARTIST) {
        return pTrack->getAlbumArtist();
    } else if (column == LIBRARYTABLE_YEAR) {
        return pTrack->getYear();
    } else if (column == LIBRARYTABLE_DATETIMEADDED) {
        return pTrack->getDateAdded();
    } else if (column == LIBRARYTABLE_GENRE) {
        return pTrack->getGenre();
    } else if (column == LIBRARYTABLE_COMPOSER) {
        return pTrack->getComposer();
    } else if (column == LIBRARYTABLE_GROUPING) {
        return pTrack->getGrouping();
    } else if (column == LIBRARYTABLE_FILETYPE) {
        return pTrack->getType();
    } else if (column == LIBRARYTABLE_TRACKNUMBER) {
        return pTrack->getTrackNumber();
    } else if (column == LIBRARYTABLE_LOCATION) {
        return pTrack->getLocation();
    } else if (column == LIBRARYTABLE_COMMENT) {
        return pTrack->getComment();
    } else if (column == LIBRARYTABLE_DURATION) {
        return pTrack->getDuration();
    } else if (column == LIBRARYTABLE_BITRATE) {
        return pTrack->getBitrate();
    } else if (column == LIBRARYTABLE_BPM) {
        return pTrack->getBpm();
    } else if (column == LIBRARYTABLE_PLAYED) {
        return pTrack->getPlayed();
    } else if (column == LIBRARYTABLE_TIMESPLAYED) {
        return pTrack->getTimesPlayed();
    } else if (column == LIBRARYTABLE_RATING) {
        return pTrack->getRating();
    } else if (column == LIBRARYTABLE_KEY) {
        return pTrack->getKeyText();
    } else if (column == LIBRARYTABLE_KEY_ID) {
        return static_cast<int>(pTrack->getKey());
    } else if (column == LIBRARYTABLE_BPM_LOCK) {
        return pTrack->hasBpmLock();
    }

    return QVariant();
}

bool AndNode::match(const TrackPointer& pTrack) const {
    if (m_nodes.isEmpty()) {
        return false;
    }

    foreach (const QueryNode* pNode, m_nodes) {
        if (!pNode->match(pTrack)) {
            return false;
        }
    }
    return true;
}

QString AndNode::toSql() const {
    QStringList queryFragments;
    foreach (const QueryNode* pNode, m_nodes) {
        QString sql = pNode->toSql();
        if (!sql.isEmpty()) {
            queryFragments << sql;
        }
    }
    return queryFragments.join(" AND ");
}

bool OrNode::match(const TrackPointer& pTrack) const {
    foreach (const QueryNode* pNode, m_nodes) {
        if (pNode->match(pTrack)) {
            return true;
        }
    }
    return false;
}

QString OrNode::toSql() const {
    QStringList queryFragments;
    foreach (const QueryNode* pNode, m_nodes) {
        QString sql = pNode->toSql();
        if (!sql.isEmpty()) {
            queryFragments << sql;
        }
    }
    return queryFragments.join(" OR ");
}

bool TextFilterNode::match(const TrackPointer& pTrack) const {
    foreach (QString sqlColumn, m_sqlColumns) {
        QVariant value = getTrackValueForColumn(pTrack, sqlColumn);
        if (!value.isValid() || !qVariantCanConvert<QString>(value)) {
            continue;
        }

        if (value.toString().contains(m_argument, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

QString TextFilterNode::toSql() const {
    FieldEscaper escaper(m_database);
    QString escapedArgument = escaper.escapeString("%" + m_argument + "%");

    QStringList searchClauses;
    foreach (QString sqlColumn, m_sqlColumns) {
        searchClauses << QString("(%1 LIKE %2)").arg(sqlColumn, escapedArgument);
    }

    return searchClauses.length() > 1 ?
            QString("(%1)").arg(searchClauses.join(" OR ")) :
            searchClauses.at(0);
}

NumericFilterNode::NumericFilterNode(const QStringList& sqlColumns,
                                     QString argument)
        : m_sqlColumns(sqlColumns),
          m_bOperatorQuery(false),
          m_operator("="),
          m_dOperatorArgument(0.0),
          m_bRangeQuery(false),
          m_dRangeLow(0.0),
          m_dRangeHigh(0.0) {
    QRegExp operatorMatcher("^(>|>=|=|<|<=)(.*)$");
    if (operatorMatcher.indexIn(argument) != -1) {
        m_operator = operatorMatcher.cap(1);
        argument = operatorMatcher.cap(2);
    }

    bool parsed = false;
    // Try to convert to see if it parses.
    m_dOperatorArgument = argument.toDouble(&parsed);
    if (parsed) {
        m_bOperatorQuery = true;
    }

    QStringList rangeArgs = argument.split("-");
    if (rangeArgs.length() == 2) {
        bool lowOk = false;
        m_dRangeLow = rangeArgs[0].toDouble(&lowOk);
        bool highOk = false;
        m_dRangeHigh = rangeArgs[1].toDouble(&highOk);

        if (lowOk && highOk && m_dRangeLow <= m_dRangeHigh) {
            m_bRangeQuery = true;
        }
    }

}

bool NumericFilterNode::match(const TrackPointer& pTrack) const {
    foreach (QString sqlColumn, m_sqlColumns) {
        QVariant value = getTrackValueForColumn(pTrack, sqlColumn);
        if (!value.isValid() || !qVariantCanConvert<double>(value)) {
            continue;
        }

        double dValue = value.toDouble();
        if (m_bOperatorQuery) {
            if ((m_operator == "=" && dValue == m_dOperatorArgument) ||
                (m_operator == "<" && dValue < m_dOperatorArgument) ||
                (m_operator == ">" && dValue > m_dOperatorArgument) ||
                (m_operator == "<=" && dValue <= m_dOperatorArgument) ||
                (m_operator == ">=" && dValue >= m_dOperatorArgument)) {
                return true;
            }
        } else if (m_bRangeQuery && dValue >= m_dRangeLow &&
                   dValue <= m_dRangeHigh) {
            return true;
        }
    }
    return false;
}

QString NumericFilterNode::toSql() const {
    if (m_bOperatorQuery) {
        QStringList searchClauses;
        foreach (const QString& sqlColumn, m_sqlColumns) {
            searchClauses << QString("(%1 %2 %3)").arg(
                sqlColumn, m_operator, QString::number(m_dOperatorArgument));
        }
        return searchClauses.length() > 1 ?
                QString("(%1)").arg(searchClauses.join(" OR ")) :
                searchClauses[0];
    }

    if (m_bRangeQuery) {
        QStringList searchClauses;
        foreach (const QString& sqlColumn, m_sqlColumns) {
            searchClauses << QString("(%1 >= %2 AND %1 <= %3)")
                    .arg(sqlColumn, QString::number(m_dRangeLow),
                         QString::number(m_dRangeHigh));
        }

        return searchClauses.length() > 1 ?
                QString("(%1)").arg(searchClauses.join(" OR ")) :
                searchClauses[0];
    }

    return QString();
}

KeyFilterNode::KeyFilterNode(mixxx::track::io::key::ChromaticKey key,
                             bool fuzzy) {
    if (fuzzy) {
        m_matchKeys = KeyUtils::getCompatibleKeys(key);
    } else {
        m_matchKeys.push_back(key);
    }
}

bool KeyFilterNode::match(const TrackPointer& pTrack) const {
    return m_matchKeys.contains(pTrack->getKey());
}

QString KeyFilterNode::toSql() const {
    QStringList searchClauses;
    foreach (mixxx::track::io::key::ChromaticKey match, m_matchKeys) {
        searchClauses << QString("(key_id IS %1)").arg(QString::number(match));
    }

    return searchClauses.length() > 1 ?
            QString("(%1)").arg(searchClauses.join(" OR ")) :
            searchClauses[0];
}
