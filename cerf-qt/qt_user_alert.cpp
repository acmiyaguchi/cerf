#include <cerfcore/host/user_alert.h>

#include "qt_main_window.h"

#include <cerfcore/core/cerf_emulator.h>

#include <QCoreApplication>
#include <QMessageBox>
#include <QMetaObject>
#include <QThread>

namespace {

class QtUserAlert : public cerfcore::UserAlert {
public:
    using cerfcore::UserAlert::UserAlert;

    void Show(cerfcore::AlertLevel level, cerfcore::AlertPlacement placement,
              const std::string& title, const std::string& text) override {
        QCoreApplication* app = QCoreApplication::instance();
        if (QThread::currentThread() == app->thread()) {
            ShowOnGuiThread(level, placement, title, text);
            return;
        }
        QMetaObject::invokeMethod(app, [&] { ShowOnGuiThread(level, placement, title, text); },
                                  Qt::BlockingQueuedConnection);
    }

    void Post(cerfcore::AlertLevel level, const std::string& title, const std::string& text,
              std::function<void()> after) override {
        QMetaObject::invokeMethod(
            QCoreApplication::instance(),
            [this, level, title, text, after] {
                ShowOnGuiThread(level, cerfcore::AlertPlacement::kStandalone, title, text);
                if (after) after();
            },
            Qt::QueuedConnection);
    }

private:
    void ShowOnGuiThread(cerfcore::AlertLevel level, cerfcore::AlertPlacement placement,
                         const std::string& title, const std::string& text) {
        QWidget* owner = placement == cerfcore::AlertPlacement::kOverWindow
                             ? emu_.Get<QtMainWindow>().Window()
                             : nullptr;
        QMessageBox box(level == cerfcore::AlertLevel::kError ? QMessageBox::Critical
                                                              : QMessageBox::Warning,
                        QString::fromStdString(title), QString::fromStdString(text),
                        QMessageBox::Ok, owner);
        if (placement == cerfcore::AlertPlacement::kTopmost)
            box.setWindowFlag(Qt::WindowStaysOnTopHint);
        box.exec();
    }
};

}

REGISTER_SERVICE_AS(QtUserAlert, cerfcore::UserAlert);
