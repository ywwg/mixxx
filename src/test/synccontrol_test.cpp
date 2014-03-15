#include <gtest/gtest.h>

#include <QtDebug>
#include <QScopedPointer>

#include "mixxxtest.h"
#include "engine/sync/synccontrol.h"

class SyncControlTest : public MixxxTest {
};

TEST_F(SyncControlTest, BestestBpmChange) {
    EXPECT_DOUBLE_EQ(70.0 / 60.0, SyncControl::getMultipliedSyncRate(60, 70));
    // Special case -- the adjustment for 0.5 and 1.0 are the same (but in
    // different directions).  Prefer 1.0.
    EXPECT_DOUBLE_EQ((80.0 / 60.0), SyncControl::getMultipliedSyncRate(60, 80));
    EXPECT_DOUBLE_EQ(1.0, SyncControl::getMultipliedSyncRate(60, 120));
    EXPECT_DOUBLE_EQ(1.0, SyncControl::getMultipliedSyncRate(60, 30));
}
