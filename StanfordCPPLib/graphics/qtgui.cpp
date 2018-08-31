/*
 * File: qtgui.cpp
 * ---------------
 *
 * @author Marty Stepp
 * @version 2018/08/23
 * - renamed to qtgui.cpp
 * @version 2018/07/03
 * - initial version
 */

#include "qtgui.h"
#include <QEvent>
#include <QThread>
#include "error.h"
#include "exceptions.h"
#include "gthread.h"
#include "strlib.h"
#include "private/static.h"

#define __DONT_ENABLE_QT_GRAPHICAL_CONSOLE
#include "console.h"
#undef __DONT_ENABLE_QT_GRAPHICAL_CONSOLE

#ifdef _WIN32
#  include <direct.h>   // for chdir
#else // _WIN32
#  include <unistd.h>   // for chdir
#endif // _WIN32


// QtGui members
QApplication* QtGui::_app = nullptr;
QtGui* QtGui::_instance = nullptr;

QtGui::QtGui()
        : _initialized(false) {
    connect(GEventQueue::instance(), SIGNAL(mySignal()), this, SLOT(mySlot()));
}

void QtGui::exitGraphics(int exitCode) {
    if (_app) {
// need to temporarily turn off C++ lib exit macro to call QApplication's exit method
// (NOTE: must keep in sync with exit definition in init.h)
#undef exit
        _app->quit();
        _app = nullptr;
        std::exit(exitCode);
#define exit __stanfordcpplib__exitLibrary
    } else {
        std::exit(exitCode);
    }
}

void QtGui::initializeQt() {
    if (_app) return;

    GThread::runOnQtGuiThread([this]() {
        if (!_app) {
            _app = new QApplication(_argc, _argv);
            _initialized = true;
        }
    });
}

QtGui* QtGui::instance() {
    if (!_instance) {
        _instance = new QtGui();
        GEventQueue::instance();   // create event queue on Qt GUI main thread
    }
    return _instance;
}

void QtGui::mySlot() {
    if (!GEventQueue::instance()->isEmpty()) {
        GThunk thunk = GEventQueue::instance()->peek();
        thunk();
        GEventQueue::instance()->dequeue();
    }
}

void QtGui::setArgs(int argc, char** argv) {
    _argc = argc;
    _argv = argv;
}

// this should be called by the Qt main thread
void QtGui::startBackgroundEventLoop(GThunkInt mainFunc) {
    GThread::ensureThatThisIsTheQtGuiThread("QtGui::startBackgroundEventLoop");

    // start student's main function in its own second thread
    if (!GThread::studentThreadExists()) {
        GStudentThread::startStudentThread(mainFunc);
        startEventLoop();   // begin Qt event loop on main thread
    }
}

// this should be called by the Qt main thread
void QtGui::startBackgroundEventLoopVoid(GThunk mainFunc) {
    GThread::ensureThatThisIsTheQtGuiThread("QtGui::startBackgroundEventLoop");

    // start student's main function in its own second thread
    if (!GThread::studentThreadExists()) {
        GStudentThread::startStudentThreadVoid(mainFunc);
        startEventLoop();   // begin Qt event loop on main thread
    }
}

// this should be called by the Qt main thread
void QtGui::startEventLoop() {
    GThread::ensureThatThisIsTheQtGuiThread("QtGui::startEventLoop");
    if (!_app) {
        error("QtGui::startEventLoop: need to initialize Qt first");
    }

    // start Qt event loop on main thread;
    // Qt GUI main thread blocks here until student main() is done
    int exitCode = _app->exec();

    // if I get here, it means an "exit on close" window was just closed;
    // it's time to shut down the Qt system and exit the C++ program
    exitGraphics(exitCode);
}
