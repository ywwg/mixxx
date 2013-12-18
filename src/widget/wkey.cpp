#include "widget/wkey.h"

#include "track/keyutils.h"
#include "track/keys.h"

WKey::WKey(QWidget* pParent)
        : WLabel(pParent),
          m_dOldValue(0),
          m_preferencesUpdated(ConfigKey("[Preferences]", "updated")) {
    setValue(m_dOldValue);
    connect(&m_preferencesUpdated, SIGNAL(valueChanged(double)),
            this, SLOT(preferencesUpdated(double)));
}

WKey::~WKey() {
}

void WKey::setValue(double dValue) {
    m_dOldValue = dValue;
    mixxx::track::io::key::ChromaticKey key =
            KeyUtils::keyFromNumericValue(dValue);

    if (key != mixxx::track::io::key::INVALID) {
        // Render this key with the user-provided notation.
        m_pLabel->setText(KeyUtils::keyToString(key));
    } else {
        m_pLabel->setText("");
    }
}

void WKey::preferencesUpdated(double dValue) {
    if (dValue > 0) {
        setValue(m_dOldValue);
    }
}
