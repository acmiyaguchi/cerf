#include "qt_theme.h"

#include <cerffrontend/theme_colors.h>

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/core/log.h>

#include <QApplication>
#include <QGuiApplication>
#include <QPalette>
#include <QStyleHints>

REGISTER_SERVICE(QtTheme);

namespace {

QColor Color(cerffrontend::Rgb c) { return QColor(c.r, c.g, c.b); }

}

void QtTheme::ApplyToApplication() {
    dark_ = QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
    LOG(Cerf, "QtTheme: %s color scheme\n", dark_ ? "dark" : "light");
    if (!dark_) return;

    namespace t = cerffrontend::theme;
    QApplication::setStyle(QStringLiteral("Fusion"));
    QPalette pal;
    pal.setColor(QPalette::Window, Color(t::kDarkDialog));
    pal.setColor(QPalette::WindowText, Color(t::kDarkText));
    pal.setColor(QPalette::Base, Color(t::kDarkEdit));
    pal.setColor(QPalette::AlternateBase, Color(t::kDarkBar));
    pal.setColor(QPalette::Text, Color(t::kDarkText));
    pal.setColor(QPalette::Button, Color(t::kDarkBar));
    pal.setColor(QPalette::ButtonText, Color(t::kDarkText));
    pal.setColor(QPalette::Highlight, Color(t::kDarkSelected));
    pal.setColor(QPalette::HighlightedText, Color(t::kDarkText));
    pal.setColor(QPalette::Link, Color(t::kDarkLink));
    pal.setColor(QPalette::Disabled, QPalette::WindowText, Color(t::kDarkDisabled));
    pal.setColor(QPalette::Disabled, QPalette::Text, Color(t::kDarkDisabled));
    pal.setColor(QPalette::Disabled, QPalette::ButtonText, Color(t::kDarkDisabled));
    QApplication::setPalette(pal);
}
