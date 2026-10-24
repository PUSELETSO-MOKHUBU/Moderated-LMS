#include "Database.h"

#include <iostream>
#include <utility>
#include <sqlite3.h>

Database::Database(std::string path)
    : dbPath_(std::move(path)), db_(nullptr) {
}

Database::~Database() {
    if (db_ != nullptr) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool Database::connect() {
    if (sqlite3_open(dbPath_.c_str(), &db_) != SQLITE_OK) {
        std::cerr << "Failed to open SQLite database: " << dbPath_ << std::endl;
        return false;
    }
    return true;
}

bool Database::initializeSchema() {
    if (!connect()) {
        return false;
    }

    const char* schema =
        "PRAGMA foreign_keys = ON;"
        "CREATE TABLE IF NOT EXISTS departments ("
        "  department_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name TEXT NOT NULL UNIQUE"
        ");"
        "CREATE TABLE IF NOT EXISTS users ("
        "  user_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  username TEXT NOT NULL UNIQUE,"
        "  password_hash TEXT NOT NULL,"
        "  role TEXT NOT NULL CHECK (role IN ('ADMIN','LECTURER','HOD','STUDENT')),"
        "  active INTEGER NOT NULL DEFAULT 1 CHECK (active IN (0,1)),"
        "  department_id INTEGER,"
        "  FOREIGN KEY (department_id) REFERENCES departments(department_id)"
        ");"
        "CREATE TABLE IF NOT EXISTS courses ("
        "  course_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  code TEXT NOT NULL UNIQUE,"
        "  title TEXT NOT NULL,"
        "  department_id INTEGER NOT NULL,"
        "  FOREIGN KEY (department_id) REFERENCES departments(department_id)"
        ");"
        "CREATE TABLE IF NOT EXISTS lecturer_courses ("
        "  lecturer_id INTEGER NOT NULL,"
        "  course_id INTEGER NOT NULL,"
        "  PRIMARY KEY (lecturer_id, course_id),"
        "  FOREIGN KEY (lecturer_id) REFERENCES users(user_id),"
        "  FOREIGN KEY (course_id) REFERENCES courses(course_id)"
        ");"
        "CREATE TABLE IF NOT EXISTS enrolments ("
        "  student_id INTEGER NOT NULL,"
        "  course_id INTEGER NOT NULL,"
        "  PRIMARY KEY (student_id, course_id),"
        "  FOREIGN KEY (student_id) REFERENCES users(user_id),"
        "  FOREIGN KEY (course_id) REFERENCES courses(course_id)"
        ");"
        "CREATE TABLE IF NOT EXISTS assignments ("
        "  assignment_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  course_id INTEGER NOT NULL,"
        "  title TEXT NOT NULL,"
        "  deadline TEXT NOT NULL,"
        "  max_mark REAL NOT NULL CHECK (max_mark > 0),"
        "  FOREIGN KEY (course_id) REFERENCES courses(course_id)"
        ");"
        "CREATE TABLE IF NOT EXISTS submissions ("
        "  submission_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  assignment_id INTEGER NOT NULL,"
        "  student_id INTEGER NOT NULL,"
        "  version_no INTEGER NOT NULL CHECK (version_no >= 1),"
        "  submitted_at TEXT NOT NULL,"
        "  status TEXT NOT NULL CHECK (status IN ('SUBMITTED','LATE','MARKED','MODERATED','RELEASED')),"
        "  file_path TEXT NOT NULL,"
        "  is_active INTEGER NOT NULL DEFAULT 1 CHECK (is_active IN (0,1)),"
        "  UNIQUE (assignment_id, student_id, version_no),"
        "  FOREIGN KEY (assignment_id) REFERENCES assignments(assignment_id),"
        "  FOREIGN KEY (student_id) REFERENCES users(user_id)"
        ");"
        "CREATE TABLE IF NOT EXISTS marks ("
        "  mark_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  submission_id INTEGER NOT NULL UNIQUE,"
        "  raw_mark REAL NOT NULL,"
        "  penalty REAL NOT NULL DEFAULT 0,"
        "  final_mark REAL NOT NULL,"
        "  feedback TEXT NOT NULL,"
        "  marked_by INTEGER NOT NULL,"
        "  FOREIGN KEY (submission_id) REFERENCES submissions(submission_id),"
        "  FOREIGN KEY (marked_by) REFERENCES users(user_id)"
        ");"
        "CREATE TABLE IF NOT EXISTS mark_audit ("
        "  audit_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  submission_id INTEGER NOT NULL,"
        "  changed_by INTEGER NOT NULL,"
        "  old_mark REAL,"
        "  new_mark REAL NOT NULL,"
        "  reason TEXT NOT NULL,"
        "  changed_at TEXT NOT NULL,"
        "  FOREIGN KEY (submission_id) REFERENCES submissions(submission_id),"
        "  FOREIGN KEY (changed_by) REFERENCES users(user_id)"
        ");";

    char* error = nullptr;
    if (sqlite3_exec(db_, schema, nullptr, nullptr, &error) != SQLITE_OK) {
        std::cerr << "Schema creation failed: " << error << std::endl;
        sqlite3_free(error);
        return false;
    }

    return true;
}

sqlite3* Database::raw() const {
    return db_;
}
