#include <QDebug>
#include <QSqlError>
#include "dbhandler.h"

DbHandler* DbHandler::instance = nullptr;

DbHandler::DbHandler()
{
    db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName("localhost");
    db.setDatabaseName("trains_db");
    db.setUserName("postgres");
}

DbHandler::~DbHandler()
{
    if (db.isOpen()){
        db.close();
    }
}

DbHandler *DbHandler::getInstance()
{
    if (!instance){
        instance = new DbHandler();
    }

    return instance;
}

void DbHandler::openDb()
{
    if (db.open()){
        qDebug() << "Успешное подключение к БД";
    } else {
        qDebug() << "Ошибка подключения к БД" << db.lastError().text();
    }
}
