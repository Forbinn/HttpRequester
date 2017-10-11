/*
 * Copyright 2017 
 *
 * Made by vincent leroy
 * Mail <vincent.54.leroy@gmail.com>
 */

// Project includes ------------------------------------------------------------
#include "MainWindow.hpp"

// Qt includes -----------------------------------------------------------------
#include <QApplication>
#include <QWidget>
#include <QStyleFactory>
#include <QFileInfo>

// C++ standard library includes -----------------------------------------------
#include <csignal>

namespace
{
    constexpr const auto styleName = "Adwaita-Dark";
}

int main(int ac, char * av[])
{
    std::signal(SIGINT,  [](int){ qApp->quit(); });
    std::signal(SIGTERM, [](int){ qApp->quit(); });

    QApplication app(ac, av);

    QApplication::setOrganizationDomain("vivoka.com");
    QApplication::setOrganizationName("vivoka");
    QApplication::setApplicationName(QFileInfo(av[0]).baseName());
    QApplication::setApplicationVersion("1.6");

    if (QStyleFactory::keys().contains(styleName))
        QApplication::setStyle(QStyleFactory::create(styleName));

    MainWindow w;
    w.restoreState();

    return app.exec();
}
