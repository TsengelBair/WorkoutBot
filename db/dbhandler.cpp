#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QDate>
#include "dbhandler.h"

DbHandler* DbHandler::instance = nullptr;

DbHandler::DbHandler()
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("trains_sqlite.db");
    openDb();
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
        createTables();
    } else {
        qDebug() << "Ошибка подключения к БД" << db.lastError().text();
    }
}

void DbHandler::createTables()
{
    QSqlQuery query;

    /// Создание таблицы workouts
    if (!query.exec("CREATE TABLE IF NOT EXISTS workouts ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "workout_date DATE NOT NULL)")) {
        qDebug() << "Ошибка при создании таблицы workouts:" << query.lastError();
    }

    /// Создание таблицы exercises
    if (!query.exec("CREATE TABLE IF NOT EXISTS exercises ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "exercise_name TEXT NOT NULL UNIQUE)")) {
        qDebug() << "Ошибка при создании таблицы exercises:" << query.lastError();
    }

    /// Создание таблицы sets
    if (!query.exec("CREATE TABLE IF NOT EXISTS sets ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "workout_id INTEGER NOT NULL, "
                    "exercise_id INTEGER NOT NULL, "
                    "tonnage REAL NOT NULL, "
                    "FOREIGN KEY (workout_id) REFERENCES workouts(id) ON DELETE CASCADE, "
                    "FOREIGN KEY (exercise_id) REFERENCES exercises(id) ON DELETE CASCADE)")) {
        qDebug() << "Ошибка при создании таблицы sets:" << query.lastError();
    }
}

/* Функция для получения ID упражнения (либо INSERT либо возврат уже имеющегося) */
int DbHandler::getOrInsertExerciseId(const QString &exerciseName) {
    QSqlQuery query;

    // Попробуем вставить новое упражнение
    query.prepare("INSERT INTO exercises (exercise_name) VALUES (:exerciseName)");
    query.bindValue(":exerciseName", exerciseName);

    if (!query.exec()) {
        // Если ошибка, проверяем, не существует ли уже такое упражнение
        if (query.lastError().text().contains("UNIQUE constraint failed")) {
            // Упражнение уже существует, получаем его ID
            query.prepare("SELECT id FROM exercises WHERE exercise_name = :exerciseName");
            query.bindValue(":exerciseName", exerciseName);
            if (!query.exec()) {
                qDebug() << "Ошибка при select из таблицы exercises:" << query.lastError();
                return -1;
            }
            if (query.next()) {
                return query.value(0).toInt();
            }
        } else {
            qDebug() << "Ошибка при insert в таблицу exercises:" << query.lastError();
            return -1;
        }
    }

    // Если вставка прошла успешно, получаем ID нового упражнения
    return query.lastInsertId().toInt();
}

bool DbHandler::saveTrain(const QString &date, const QMap<QString, QList<double> > &trainInfo)
{
    QSqlQuery query;
    query.prepare("INSERT INTO workouts (workout_date) VALUES (:date)");
    query.bindValue(":date", QDate::fromString(date, "dd-MM-yyyy"));

    if (!query.exec()){
        qDebug() << "Ошибка при insert в основную таблицу";
        return 0;
    }

    /// Получаем ID последней вставленной записи
    int workoutId = query.lastInsertId().toInt();

    /// INSERT в таблицу с упражнениями, получаем id упражнения и выполним в цикле insertы в таблицу sets
    for (const auto &exercise : trainInfo.keys()) {
        int exerciseId = getOrInsertExerciseId(exercise);
        if (exerciseId == -1) {
//            qDebug() << "Ошибка с id упражнения";
//            continue;
            return 0;
        }

        /// Вставляем данные в таблицу sets
        for (double tonnage : trainInfo[exercise]) {
            QSqlQuery setQuery;
            setQuery.prepare("INSERT INTO sets (workout_id, exercise_id, tonnage) VALUES (:workoutId, :exerciseId, :tonnage)");
            setQuery.bindValue(":workoutId", workoutId);
            setQuery.bindValue(":exerciseId", exerciseId);
            setQuery.bindValue(":tonnage", tonnage);

            if (!setQuery.exec()) {
                qDebug() << "Ошибка при insert в таблицу sets:" << setQuery.lastError();
                return 0;
            }
        }
    }

    return 1;
}

QMap<QString, double> DbHandler::trainData()
{
    QMap<QString, double> data;

    QSqlQuery query;
    QString queryStr = "SELECT workouts.id, workouts.workout_date, SUM(sets.tonnage) "
                       "FROM workouts "
                       "JOIN sets ON workouts.id = sets.workout_id "
                       "GROUP BY workouts.id, workouts.workout_date "
                       "ORDER BY workouts.workout_date;";

    if (!query.exec(queryStr)) {
        qDebug() << "Ошибка выполнения запроса:" << query.lastError().text();
        return data;
    }

    while(query.next()){
        QString workoutDate = query.value(1).toString();
        double totalTonnage = query.value(2).toDouble();

        data.insert(workoutDate, totalTonnage);
    }

    return data;
}

QSqlDatabase &DbHandler::getDb()
{
    if (!instance){
        throw std::runtime_error("Db is not open!");
    }

    return db;
}
