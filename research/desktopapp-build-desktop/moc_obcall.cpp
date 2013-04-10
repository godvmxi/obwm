/****************************************************************************
** Meta object code from reading C++ file 'obcall.h'
**
** Created: Wed Apr 10 11:30:36 2013
**      by: The Qt Meta Object Compiler version 62 (Qt 4.7.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../desktopapp/obcall.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'obcall.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 62
#error "This file was generated using the moc from 4.7.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ObCall[] = {

 // content:
       5,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      11,    8,    7,    7, 0x05,

 // slots: signature, parameters, type, tag, flags
      47,    7,    7,    7, 0x0a,
      69,   61,    7,    7, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_ObCall[] = {
    "ObCall\0\0,,\0returnDate(APP_COM,QString,QString)\0"
    "receiveData()\0appinfo\0call(APP_COM)\0"
};

const QMetaObject ObCall::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_ObCall,
      qt_meta_data_ObCall, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ObCall::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ObCall::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ObCall::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ObCall))
        return static_cast<void*>(const_cast< ObCall*>(this));
    return QObject::qt_metacast(_clname);
}

int ObCall::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: returnDate((*reinterpret_cast< APP_COM(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2])),(*reinterpret_cast< QString(*)>(_a[3]))); break;
        case 1: receiveData(); break;
        case 2: call((*reinterpret_cast< APP_COM(*)>(_a[1]))); break;
        default: ;
        }
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void ObCall::returnDate(APP_COM _t1, QString _t2, QString _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
