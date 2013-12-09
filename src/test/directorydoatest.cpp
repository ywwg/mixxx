#include <gtest/gtest.h>

#include <QtDebug>
#include <QtSql>
#include <QString>
#include <QStringBuilder>
#include <QDir>
#include <QFileInfo>

#include "configobject.h"
#include "library/dao/directorydao.h"
#include "library/dao/trackdao.h"
#include "library/trackcollection.h"
#include "test/mixxxtest.h"

namespace {

class DirectoryDAOTest : public MixxxTest {
  protected:
    virtual void SetUp() {
        // make sure to use the current schema.xml file in the repo
        config()->set(ConfigKey("[Config]","Path"),
                      QDir::currentPath().append("/res"));
        m_pTrackCollection = new TrackCollection(config());
    }

    virtual void TearDown() {
        // make sure we clean up the db
        QSqlQuery query(m_pTrackCollection->getDatabase());
        query.prepare("DELETE FROM " % DIRECTORYDAO_TABLE);
        query.exec();
        query.prepare("DELETE FROM library");
        query.exec();
        query.prepare("DELETE FROM track_locations");
        query.exec();

        delete m_pTrackCollection;
    }

    TrackCollection *m_pTrackCollection;
};

TEST_F(DirectoryDAOTest, addDirTest) {
    DirectoryDAO m_DirectoryDao = m_pTrackCollection->getDirectoryDAO();
    // prepend dir with '/' so that QT thinks the dir starts at the root
    QString testdir(QDir::tempPath() + "/TestDir/a");
    QString testChild(QDir::tempPath() + "/TestDir/a/child");
    QString testParent(QDir::tempPath() + "/TestDir");

    //create temp dirs
    QDir(QDir::temp()).mkpath(testParent);
    QDir(QDir::temp()).mkpath(testdir);
    QDir(QDir::temp()).mkpath(testChild);

    // check if directory doa adds and thinks everything is ok
    int success = m_DirectoryDao.addDirectory(testdir);
    EXPECT_EQ(success, ALL_FINE );

    // check that we don't add the directory again
    success = m_DirectoryDao.addDirectory(testdir);
    EXPECT_EQ(success, SQL_ERROR );

    // check that we don't add a child directory
    success = m_DirectoryDao.addDirectory(testChild);
    EXPECT_EQ(success, ALREADY_WATCHING );

    // check that we add the parent dir
    success = m_DirectoryDao.addDirectory(testParent);
    EXPECT_EQ(success, ALL_FINE);

    QSqlQuery query(m_pTrackCollection->getDatabase());
    query.prepare("SELECT " % DIRECTORYDAO_DIR % " FROM " % DIRECTORYDAO_TABLE);
    success = query.exec();

    // we do not trust what directory dao thinks and better check up on it
    QStringList dirs;
    while (query.next()) {
        dirs << query.value(0).toString();
    }

    // the test db should be always empty when tests are started.
    EXPECT_TRUE(dirs.size() == 1);
    if (dirs.size() > 0) {
        EXPECT_TRUE(dirs.at(0) == testParent);
    }
}

TEST_F(DirectoryDAOTest, removeDirTest) {
    DirectoryDAO m_DirectoryDao = m_pTrackCollection->getDirectoryDAO();
    QString testdir = QDir::currentPath().append("/src/test/test_data");

    // check if directory doa adds and thinks everything is ok
    m_DirectoryDao.addDirectory(testdir);

    int success = m_DirectoryDao.removeDirectory(testdir);
    EXPECT_EQ(success, ALL_FINE);

    // we do not trust what directory dao thinks and better check up on it
    QSqlQuery query(m_pTrackCollection->getDatabase());
    query.prepare("SELECT " % DIRECTORYDAO_DIR % " FROM " % DIRECTORYDAO_TABLE);
    success = query.exec();
    QStringList dirs;
    while (query.next()) {
        dirs << query.value(0).toString();
    }

    // the db should have now no entries left anymore
    EXPECT_TRUE(dirs.size() == 0);
}

TEST_F(DirectoryDAOTest, getDirTest) {
    DirectoryDAO m_DirectoryDao = m_pTrackCollection->getDirectoryDAO();
    QString testdir = "/a/c";
    QString testdir2 = "b/d";

    m_DirectoryDao.addDirectory(testdir);
    m_DirectoryDao.addDirectory(testdir2);

    QStringList dirs = m_DirectoryDao.getDirs();

    EXPECT_TRUE(dirs.size() == 2);
    if (dirs.size() == 2) {
        EXPECT_TRUE(dirs.at(0) == testdir);
        EXPECT_TRUE(dirs.at(1) == testdir2);
    }
}

TEST_F(DirectoryDAOTest, relocateDirTest) {
    DirectoryDAO &directoryDao = m_pTrackCollection->getDirectoryDAO();

    directoryDao.addDirectory("/Test");
    directoryDao.addDirectory("/Test2");
    TrackDAO &trackDAO = m_pTrackCollection->getTrackDAO();
    // ok now lets create some tracks here
    trackDAO.addTracksPrepare();
    trackDAO.addTracksAdd(new TrackInfoObject("/Test/a", false), false);
    trackDAO.addTracksAdd(new TrackInfoObject("/Test/b", false), false);
    trackDAO.addTracksAdd(new TrackInfoObject("/Test2/c", false), false);
    trackDAO.addTracksAdd(new TrackInfoObject("/Test2/d", false), false);
    trackDAO.addTracksFinish(false);

    QSet<int> ids = directoryDao.relocateDirectory("/Test", "/new");
    EXPECT_EQ(ids.size(), 2);

    QStringList dirs = directoryDao.getDirs();
    EXPECT_TRUE(dirs.size() == 2);
    if (dirs.size() == 2) {
        EXPECT_TRUE(dirs.at(0) == "/new");
        EXPECT_TRUE(dirs.at(1) == "/Test2");
    }
}

}  // namespace
