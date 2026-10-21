#include <cerffrontend/image_exporter.h>

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/core/log.h>

#include <QClipboard>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QImage>
#include <QMetaObject>
#include <QThread>

namespace {

QImage Wrap(const std::vector<uint32_t>& pixels, uint32_t w, uint32_t h) {
    return QImage(reinterpret_cast<const uchar*>(pixels.data()), static_cast<int>(w),
                  static_cast<int>(h), static_cast<qsizetype>(w) * 4, QImage::Format_RGB32);
}

class QtImageExporter : public cerffrontend::ImageExporter {
public:
    using cerffrontend::ImageExporter::ImageExporter;

    bool WritePng(const std::vector<uint32_t>& pixels, uint32_t w, uint32_t h,
                  const std::string& path) override {
        if (pixels.empty() || w == 0 || h == 0) return false;
        if (Wrap(pixels, w, h).save(QString::fromStdString(path), "PNG")) return true;
        LOG(Caution, "QtImageExporter: PNG %s not written\n", path.c_str());
        return false;
    }

    bool CopyToClipboard(const std::vector<uint32_t>& pixels, uint32_t w, uint32_t h,
                         uint64_t) override {
        if (pixels.empty() || w == 0 || h == 0) return false;
        QImage            copy = Wrap(pixels, w, h).copy();
        QCoreApplication* app  = QCoreApplication::instance();
        if (QThread::currentThread() == app->thread()) {
            QGuiApplication::clipboard()->setImage(copy);
            return true;
        }
        QMetaObject::invokeMethod(app, [&copy] { QGuiApplication::clipboard()->setImage(copy); },
                                  Qt::BlockingQueuedConnection);
        return true;
    }
};

}

REGISTER_SERVICE_AS(QtImageExporter, cerffrontend::ImageExporter);
