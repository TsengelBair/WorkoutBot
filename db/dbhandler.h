#ifndef DBHANDLER_H
#define DBHANDLER_H

#include <QSqlDatabase>

class DbHandler
{
private:
    DbHandler();
    ~DbHandler();

    QSqlDatabase db;
    static DbHandler* instance;

public:
    static DbHandler* getInstance();
    void openDb();

};

#endif // DBHANDLER_H
