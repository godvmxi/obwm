/****************************************************************************
** Meta object code from reading C++ file 'appmanager.h'
**
** Created: Wed Apr 10 14:49:39 2013
**      by: The Qt Meta Object Compiler version 62 (Qt 4.7.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../desktopapp/appmanager.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'appmanager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 62
#error "This file was generated using the moc from 4.7.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_AppManager[] = {

 // content:
       5,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      21,   17,   12,   11, 0x05,

 // slots: signature, parameters, type, tag, flags
      50,   46,   12,   11, 0x0a,
      76,   67,   12,   11, 0x0a,
     122,  118,   12,   11, 0x0a,
     151,  118,   12,   11, 0x0a,
     180,  118,   12,   11, 0x0a,
     211,  118,   12,   11, 0x0a,
     259,  242,   12,   11, 0x0a,
     325,  309,   12,   11, 0x0a,
     367,   11,   12,   11, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_AppManager[] = {
    "AppManager\0\0bool\0com\0execObCmdSignal(APP_COM)\0"
    "ptr\0iconClick(void*)\0newState\0"
    "appProcessChanged(QProcess::ProcessState)\0"
    "app\0homeButtonHomeMsgSlot(void*)\0"
    "homeButtonBackMsgSlot(void*)\0"
    "homeButtonResizeMsgSlot(void*)\0"
    "homeButtonScreenMsgSlot(void*)\0"
    "coms,state,error\0"
    "execObAppsCmdSlot(QList<APP_COM>,QString,QString)\0"
    "com,state,error\0"
    "execObAppCmdSlot(APP_COM,QString,QString)\0"
    "execObCmdTimeout()\0"
};

const QMetaObject AppManager::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_AppManager,
      qt_meta_data_AppManager, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &AppManager::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *AppManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *AppManager::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_AppManager))
        return static_cast<void*>(const_cast< AppManager*>(this));
    return QWidget::qt_metacast(_clname);
}

int AppManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: { bool _r = execObCmdSignal((*reinterpret_cast< APP_COM(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 1: { bool _r = iconClick((*reinterpret_cast< void*(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 2: { bool _r = appProcessChanged((*reinterpret_cast< QProcess::ProcessState(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 3: { bool _r = homeButtonHomeMsgSlot((*reinterpret_cast< void*(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 4: { bool _r = homeButtonBackMsgSlot((*reinterpret_cast< void*(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 5: { bool _r = homeButtonResizeMsgSlot((*reinterpret_cast< void*(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 6: { bool _r = homeButtonScreenMsgSlot((*reinterpret_cast< void*(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 7: { bool _r = execObAppsCmdSlot((*reinterpret_cast< QList<APP_COM>(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2])),(*reinterpret_cast< QString(*)>(_a[3])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 8: { bool _r = execObAppCmdSlot((*reinterpret_cast< APP_COM(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2])),(*reinterpret_cast< QString(*)>(_a[3])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 9: { bool _r = execObCmdTimeout();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        default: ;
        }
        _id -= 10;
    }
    return _id;
}

// SIGNAL 0
bool AppManager::execObCmdSignal(APP_COM _t1)
{
    bool _t0;
    void *_a[] = { const_cast<void*>(reinterpret_cast<const void*>(&_t0)), const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
    return _t0;
}
QT_END_MOC_NAMESPACE
