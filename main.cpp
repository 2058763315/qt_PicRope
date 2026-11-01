// =====================================================================
//  main.cpp — 图绳 PicRope（C++/Qt 重构版）入口
//  原 C#/WPF 项目：D:\daima\0001\PicRope
// =====================================================================

#include <QApplication>
#include <QFontDatabase>
#include <QIcon>
#include <QLockFile>
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>

#include "app.h"
#include "state.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("PicRope"));
    QApplication::setApplicationDisplayName(QStringLiteral("图绳 PicRope"));
    QApplication::setOrganizationName(QStringLiteral("PicRope"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/app.ico")));

    // 统一界面字体：优先用中文界面字体，避免自绘文字在英文默认字体下走回退
    {
        const QStringList families = QFontDatabase::families();
        QFont f = QApplication::font();
        for (const QString &name : { QStringLiteral("Microsoft YaHei UI"),
                                     QStringLiteral("Microsoft YaHei"),
                                     QStringLiteral("Segoe UI") }) {
            if (families.contains(name)) { f.setFamily(name); break; }
        }
        QApplication::setFont(f);
    }

    // 关闭最后一个窗口时不退出：绳子本身就是隐藏的常驻窗口，退出只走托盘菜单
    QApplication::setQuitOnLastWindowClosed(false);

    // 单实例：崩溃后留下的锁文件可被清理，避免再也无法启动
    const QString lockPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
        + QStringLiteral("/picrope.lock");
    QLockFile lock(lockPath);
    if (!lock.tryLock(100)) {
        lock.removeStaleLockFile();
        if (!lock.tryLock(100)) {
            QMessageBox::information(nullptr, QStringLiteral("图绳"),
                                     QStringLiteral("图绳已在运行。"));
            return 0;
        }
    }

    // 状态目录：程序旁 data/
    StateStore store(QCoreApplication::applicationDirPath() + QStringLiteral("/data"));
    store.load();

    AppController controller(&store);
    if (!controller.start()) {
        QMessageBox::critical(nullptr, QStringLiteral("图绳"),
                              QStringLiteral("图绳启动失败，请查看 data/error.log。"));
        return 1;
    }

    return app.exec();
}
