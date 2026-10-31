#include "Database.h"

#include <sqlite3.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "LatePenaltyStrategy.h"
#include "PasswordHasher.h"

namespace {

class Statement {
public:
    Statement(sqlite3* db, const std::string& sql) : db_(db), stmt_(nullptr) {
        const int result = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt_, nullptr);
        if (result != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(db_));
        }
    }

    ~Statement() { sqlite3_finalize(stmt_); }
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    void text(int index, const std::string& value) {
        if (sqlite3_bind_text(stmt_, index, value.c_str(), static_cast<int>(value.size()), SQLITE_TRANSIENT) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(db_));
        }
    }

    void integer(int index, int value) {
        if (sqlite3_bind_int(stmt_, index, value) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(db_));
        }
    }

    void real(int index, double value) {
        if (sqlite3_bind_double(stmt_, index, value) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(db_));
        }
    }

    void null(int index) {
        if (sqlite3_bind_null(stmt_, index) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(db_));
        }
    }

    int step() { return sqlite3_step(stmt_); }
    int intColumn(int column) const { return sqlite3_column_int(stmt_, column); }
    double realColumn(int column) const { return sqlite3_column_double(stmt_, column); }
    bool isNull(int column) const { return sqlite3_column_type(stmt_, column) == SQLITE_NULL; }
    std::string stringColumn(int column) const {
        const auto* value = sqlite3_column_text(stmt_, column);
        return value == nullptr ? std::string() : reinterpret_cast<const char*>(value);
    }

private:
    sqlite3* db_;
    sqlite3_stmt* stmt_;
};

void checkDone(Statement& statement, sqlite3* db) {
    if (statement.step() != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }
}

void execute(sqlite3* db, const std::string& sql) {
    char* error = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &error) != SQLITE_OK) {
        const std::string message = error == nullptr ? sqlite3_errmsg(db) : error;
        sqlite3_free(error);
        throw std::runtime_error(message);
    }
}

void ensureColumn(sqlite3* db, const std::string& table, const std::string& column,
                  const std::string& definition) {
    Statement columns(db, "PRAGMA table_info(" + table + ");");
    while (columns.step() == SQLITE_ROW) {
        if (columns.stringColumn(1) == column) return;
    }
    execute(db, "ALTER TABLE " + table + " ADD COLUMN " + column + " " + definition + ";");
}

class Transaction {
public:
    explicit Transaction(sqlite3* db) : db_(db), committed_(false) {
        execute(db_, "BEGIN IMMEDIATE;");
    }
    ~Transaction() {
        if (!committed_) {
            sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        }
    }
    void commit() {
        execute(db_, "COMMIT;");
        committed_ = true;
    }

private:
    sqlite3* db_;
    bool committed_;
};

void requireCurrentIdentity(sqlite3* db, const User& actor) {
    Statement query(db, "SELECT role,active,COALESCE(department_id,-1) FROM users WHERE user_id=?;");
    query.integer(1, actor.id());
    if (query.step() != SQLITE_ROW || query.intColumn(1) != 1 ||
        query.stringColumn(0) != roleToText(actor.role()) ||
        query.intColumn(2) != actor.departmentId()) {
        throw std::runtime_error("The account is disabled or its role/department changed. Sign in again.");
    }
}

void requirePermission(sqlite3* db, const User& actor, Permission permission) {
    requireCurrentIdentity(db, actor);
    if (!actor.can(permission)) {
        throw std::runtime_error("Access denied for this role.");
    }
}

void requireText(const std::string& value, const std::string& field) {
    if (value.empty() || value.find_first_not_of(" \t\r\n") == std::string::npos) {
        throw std::invalid_argument(field + " must not be empty.");
    }
    if (value.size() > 1000) {
        throw std::invalid_argument(field + " is too long.");
    }
}

bool toLocalTime(const std::time_t& timestamp, std::tm& result) {
#if defined(_WIN32)
    return localtime_s(&result, &timestamp) == 0;
#else
    return localtime_r(&timestamp, &result) != nullptr;
#endif
}

std::tm parseDateTime(const std::string& value) {
    std::tm parsed = {};
    std::istringstream input(value);
    input >> std::get_time(&parsed, "%Y-%m-%d %H:%M");
    if (input.fail() || input.peek() != std::char_traits<char>::eof()) {
        throw std::invalid_argument("Date/time must use YYYY-MM-DD HH:MM.");
    }
    parsed.tm_sec = 0;
    parsed.tm_isdst = -1;
    const std::tm original = parsed;
    const std::time_t timestamp = std::mktime(&parsed);
    if (timestamp == static_cast<std::time_t>(-1)) {
        throw std::invalid_argument("Date/time is invalid.");
    }
    std::tm checked = {};
    if (!toLocalTime(timestamp, checked)) {
        throw std::invalid_argument("Date/time is invalid.");
    }
    if (checked.tm_year != original.tm_year || checked.tm_mon != original.tm_mon ||
        checked.tm_mday != original.tm_mday || checked.tm_hour != original.tm_hour ||
        checked.tm_min != original.tm_min) {
        throw std::invalid_argument("Date/time is invalid.");
    }
    return checked;
}

std::string normalizeDateTime(const std::string& value) {
    const std::tm parsed = parseDateTime(value);
    std::ostringstream output;
    output << std::put_time(&parsed, "%Y-%m-%dT%H:%M:%S");
    return output.str();
}

std::time_t toTimestamp(std::string value) {
    if (value.size() >= 16 && value[10] == 'T') {
        value[10] = ' ';
        value.resize(16);
    }
    std::tm parsed = parseDateTime(value);
    const std::time_t timestamp = std::mktime(&parsed);
    if (timestamp == static_cast<std::time_t>(-1)) {
        throw std::runtime_error("Stored date/time is invalid.");
    }
    return timestamp;
}

std::string currentDateTime() {
    const std::time_t now = std::time(nullptr);
    std::tm local = {};
    if (!toLocalTime(now, local)) {
        throw std::runtime_error("Unable to determine the local date/time.");
    }
    std::ostringstream output;
    output << std::put_time(&local, "%Y-%m-%dT%H:%M:%S");
    return output.str();
}

int daysLate(const std::string& submittedAt, const std::string& deadline) {
    const double seconds = std::difftime(toTimestamp(submittedAt), toTimestamp(deadline));
    if (seconds <= 0.0) {
        return 0;
    }
    return static_cast<int>(std::ceil(seconds / (24.0 * 60.0 * 60.0)));
}

void bindActorId(Statement& statement, int index, const User& actor) {
    statement.integer(index, actor.id());
}

SubmissionRecord readSubmission(Statement& statement) {
    return {statement.intColumn(0), statement.intColumn(1), statement.intColumn(2),
            statement.intColumn(3), statement.stringColumn(4), statement.stringColumn(5),
            statement.stringColumn(6), statement.stringColumn(7), statement.stringColumn(8),
            statement.stringColumn(9), statement.stringColumn(10), statement.realColumn(11),
            statement.realColumn(12), statement.realColumn(13), statement.stringColumn(14)};
}

const char* submissionSelect =
    "SELECT s.submission_id, s.assignment_id, s.student_id, s.version_no, s.submitted_at, "
    "s.status, s.file_path, a.title, c.code, a.attachment_path, u.username, a.max_mark, "
    "COALESCE(m.penalty, 0), COALESCE(m.final_mark, 0), COALESCE(m.feedback, '') "
    "FROM submissions s JOIN assignments a ON a.assignment_id=s.assignment_id "
    "JOIN courses c ON c.course_id=a.course_id JOIN users u ON u.user_id=s.student_id ";

}  // namespace

struct Database::Impl {
    sqlite3* connection = nullptr;
};

Database::Database(std::string path) : dbPath_(std::move(path)), impl_(new Impl()) {
    if (sqlite3_open(dbPath_.c_str(), &impl_->connection) != SQLITE_OK) {
        const std::string message = impl_->connection == nullptr
                                        ? "Unable to open SQLite database."
                                        : sqlite3_errmsg(impl_->connection);
        if (impl_->connection != nullptr) {
            sqlite3_close(impl_->connection);
            impl_->connection = nullptr;
        }
        throw std::runtime_error("Unable to open database: " + message);
    }
    sqlite3_busy_timeout(impl_->connection, 5000);
    execute(impl_->connection, "PRAGMA foreign_keys=ON;");
}

Database::~Database() {
    if (impl_ && impl_->connection != nullptr) {
        sqlite3_close(impl_->connection);
    }
}

void Database::initializeSchema() {
    const char* schema =
        "CREATE TABLE IF NOT EXISTS departments (department_id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL UNIQUE);"
        "CREATE TABLE IF NOT EXISTS users (user_id INTEGER PRIMARY KEY AUTOINCREMENT, username TEXT NOT NULL UNIQUE, "
        "password_hash TEXT NOT NULL, role TEXT NOT NULL CHECK(role IN ('ADMIN','LECTURER','HOD','STUDENT')), "
        "active INTEGER NOT NULL DEFAULT 1 CHECK(active IN (0,1)), department_id INTEGER, "
        "FOREIGN KEY(department_id) REFERENCES departments(department_id));"
        "CREATE TABLE IF NOT EXISTS courses (course_id INTEGER PRIMARY KEY AUTOINCREMENT, code TEXT NOT NULL UNIQUE, "
        "title TEXT NOT NULL, department_id INTEGER NOT NULL, FOREIGN KEY(department_id) REFERENCES departments(department_id));"
        "CREATE TABLE IF NOT EXISTS lecturer_courses (lecturer_id INTEGER NOT NULL, course_id INTEGER NOT NULL, "
        "PRIMARY KEY(lecturer_id,course_id), FOREIGN KEY(lecturer_id) REFERENCES users(user_id), "
        "FOREIGN KEY(course_id) REFERENCES courses(course_id));"
        "CREATE TABLE IF NOT EXISTS enrolments (student_id INTEGER NOT NULL, course_id INTEGER NOT NULL, "
        "PRIMARY KEY(student_id,course_id), FOREIGN KEY(student_id) REFERENCES users(user_id), "
        "FOREIGN KEY(course_id) REFERENCES courses(course_id));"
        "CREATE TABLE IF NOT EXISTS assignments (assignment_id INTEGER PRIMARY KEY AUTOINCREMENT, course_id INTEGER NOT NULL, "
        "title TEXT NOT NULL, deadline TEXT NOT NULL, max_mark REAL NOT NULL CHECK(max_mark>0), "
        "FOREIGN KEY(course_id) REFERENCES courses(course_id));"
        "CREATE TABLE IF NOT EXISTS extensions (extension_id INTEGER PRIMARY KEY AUTOINCREMENT, assignment_id INTEGER NOT NULL, "
        "student_id INTEGER NOT NULL, new_deadline TEXT NOT NULL, reason TEXT NOT NULL, granted_by INTEGER NOT NULL, "
        "UNIQUE(assignment_id,student_id), FOREIGN KEY(assignment_id) REFERENCES assignments(assignment_id), "
        "FOREIGN KEY(student_id) REFERENCES users(user_id), FOREIGN KEY(granted_by) REFERENCES users(user_id));"
        "CREATE TABLE IF NOT EXISTS submissions (submission_id INTEGER PRIMARY KEY AUTOINCREMENT, assignment_id INTEGER NOT NULL, "
        "student_id INTEGER NOT NULL, version_no INTEGER NOT NULL CHECK(version_no>=1), submitted_at TEXT NOT NULL, "
        "status TEXT NOT NULL CHECK(status IN ('SUBMITTED','LATE','MARKED','MODERATED','RELEASED')), "
        "file_path TEXT NOT NULL, is_active INTEGER NOT NULL DEFAULT 1 CHECK(is_active IN (0,1)), "
        "UNIQUE(assignment_id,student_id,version_no), FOREIGN KEY(assignment_id) REFERENCES assignments(assignment_id), "
        "FOREIGN KEY(student_id) REFERENCES users(user_id));"
        "CREATE TABLE IF NOT EXISTS marks (mark_id INTEGER PRIMARY KEY AUTOINCREMENT, submission_id INTEGER NOT NULL UNIQUE, "
        "raw_mark REAL NOT NULL, penalty REAL NOT NULL DEFAULT 0, final_mark REAL NOT NULL, feedback TEXT NOT NULL, "
        "marked_by INTEGER NOT NULL, FOREIGN KEY(submission_id) REFERENCES submissions(submission_id), "
        "FOREIGN KEY(marked_by) REFERENCES users(user_id));"
        "CREATE TABLE IF NOT EXISTS mark_audit (audit_id INTEGER PRIMARY KEY AUTOINCREMENT, submission_id INTEGER NOT NULL, "
        "changed_by INTEGER NOT NULL, old_mark REAL, new_mark REAL NOT NULL, reason TEXT NOT NULL, changed_at TEXT NOT NULL, "
        "FOREIGN KEY(submission_id) REFERENCES submissions(submission_id), FOREIGN KEY(changed_by) REFERENCES users(user_id));"
        "CREATE INDEX IF NOT EXISTS idx_assignments_course ON assignments(course_id);"
        "CREATE INDEX IF NOT EXISTS idx_submissions_lookup ON submissions(assignment_id,student_id,status);"
        "CREATE UNIQUE INDEX IF NOT EXISTS idx_one_active_submission ON submissions(assignment_id,student_id) WHERE is_active=1;"
        "CREATE INDEX IF NOT EXISTS idx_mark_audit_submission ON mark_audit(submission_id);"
        "CREATE INDEX IF NOT EXISTS idx_extensions_student ON extensions(student_id);"
        "CREATE INDEX IF NOT EXISTS idx_marks_marker ON marks(marked_by);"
        "CREATE TRIGGER IF NOT EXISTS mark_audit_no_update BEFORE UPDATE ON mark_audit BEGIN "
        "SELECT RAISE(ABORT,'mark audit records are immutable'); END;"
        "CREATE TRIGGER IF NOT EXISTS mark_audit_no_delete BEFORE DELETE ON mark_audit BEGIN "
        "SELECT RAISE(ABORT,'mark audit records are immutable'); END;";
    execute(impl_->connection, schema);
    ensureColumn(impl_->connection, "assignments", "attachment_path", "TEXT NOT NULL DEFAULT ''");

    // Upgrade databases created by earlier starter builds that stored passwords directly.
    Transaction transaction(impl_->connection);
    Statement users(impl_->connection, "SELECT user_id,password_hash FROM users;");
    std::vector<std::pair<int, std::string>> legacy;
    while (users.step() == SQLITE_ROW) {
        const std::string stored = users.stringColumn(1);
        if (!PasswordHasher::isEncodedHash(stored)) {
            legacy.emplace_back(users.intColumn(0), stored);
        }
    }
    for (const auto& user : legacy) {
        Statement update(impl_->connection, "UPDATE users SET password_hash=? WHERE user_id=?;");
        update.text(1, PasswordHasher::hash(user.second));
        update.integer(2, user.first);
        checkDone(update, impl_->connection);
    }
    transaction.commit();
}

void Database::seedDemoAccounts() {
    sqlite3* db = impl_->connection;
    Transaction transaction(db);
    Statement department(db, "INSERT OR IGNORE INTO departments(name) VALUES(?);");
    department.text(1, "General");
    checkDone(department, db);
    Statement departmentIdQuery(db, "SELECT department_id FROM departments WHERE name=?;");
    departmentIdQuery.text(1, "General");
    if (departmentIdQuery.step() != SQLITE_ROW) {
        throw std::runtime_error("Unable to create demo department.");
    }
    const int departmentId = departmentIdQuery.intColumn(0);

    struct Seed { const char* username; const char* password; const char* role; };
    const Seed seeds[] = {
        {"admin", "admin123", "ADMIN"}, {"lecturer", "lecturer123", "LECTURER"},
        {"hod", "hod123", "HOD"}, {"student", "student123", "STUDENT"}
    };
    for (const auto& seed : seeds) {
        Statement existing(db, "SELECT 1 FROM users WHERE username=? COLLATE NOCASE;");
        existing.text(1, seed.username);
        if (existing.step() == SQLITE_ROW) continue;
        Statement add(db, "INSERT INTO users(username,password_hash,role,active,department_id) VALUES(?,?,?,1,?);");
        add.text(1, seed.username);
        add.text(2, PasswordHasher::hash(seed.password));
        add.text(3, seed.role);
        add.integer(4, departmentId);
        checkDone(add, db);
    }

    Statement repairDemoHod(db, "UPDATE users SET department_id=? WHERE username=? COLLATE NOCASE "
                                 "AND role='HOD' AND department_id IS NULL;");
    repairDemoHod.integer(1, departmentId);
    repairDemoHod.text(2, "hod");
    checkDone(repairDemoHod, db);

    Statement course(db, "INSERT OR IGNORE INTO courses(code,title,department_id) VALUES(?,?,?);");
    course.text(1, "DEMO101");
    course.text(2, "Demonstration Course");
    course.integer(3, departmentId);
    checkDone(course, db);
    Statement courseQuery(db, "SELECT course_id FROM courses WHERE code=?;");
    courseQuery.text(1, "DEMO101");
    if (courseQuery.step() != SQLITE_ROW) {
        throw std::runtime_error("Unable to create demo course.");
    }
    const int courseId = courseQuery.intColumn(0);
    Statement lecturer(db, "INSERT OR IGNORE INTO lecturer_courses(lecturer_id,course_id) "
                           "SELECT user_id,? FROM users WHERE username=? AND role='LECTURER';");
    lecturer.integer(1, courseId);
    lecturer.text(2, "lecturer");
    checkDone(lecturer, db);
    Statement student(db, "INSERT OR IGNORE INTO enrolments(student_id,course_id) "
                          "SELECT user_id,? FROM users WHERE username=? AND role='STUDENT';");
    student.integer(1, courseId);
    student.text(2, "student");
    checkDone(student, db);
    transaction.commit();
}

std::unique_ptr<User> Database::authenticate(const std::string& username, const std::string& password) {
    requireText(username, "Username");
    requireText(password, "Password");
    std::string trimmedUsername = username;
    const std::size_t first = trimmedUsername.find_first_not_of(" \t\r\n");
    const std::size_t last = trimmedUsername.find_last_not_of(" \t\r\n");
    trimmedUsername = first == std::string::npos ? std::string() :
                      trimmedUsername.substr(first, last - first + 1);
    if (trimmedUsername.empty()) throw std::invalid_argument("Username must not be empty.");

    const auto findAccount = [this, &password](const std::string& candidate) -> std::unique_ptr<User> {
        Statement query(impl_->connection,
                        "SELECT user_id,username,password_hash,role,department_id FROM users "
                        "WHERE username=? COLLATE NOCASE AND active=1 LIMIT 1;");
        query.text(1, candidate);
        if (query.step() != SQLITE_ROW || !PasswordHasher::verify(password, query.stringColumn(2))) {
            return nullptr;
        }
        return UserFactory::create(query.intColumn(0), query.stringColumn(1), query.stringColumn(2),
                                   query.stringColumn(3), query.isNull(4) ? -1 : query.intColumn(4));
    };

    if (auto account = findAccount(trimmedUsername)) return account;
    std::string alias = trimmedUsername;
    std::transform(alias.begin(), alias.end(), alias.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    if (alias == "lecture") return findAccount("lecturer");
    if (alias == "head") return findAccount("hod");
    return nullptr;
}

int Database::createUser(const User& actor, const std::string& username, const std::string& password,
                         Role role, int departmentId) {
    requirePermission(impl_->connection, actor, Permission::ManageUsers);
    requireText(username, "Username");
    requireText(password, "Password");
    if (username.size() > 64 || password.size() < 8) {
        throw std::invalid_argument("Username must be at most 64 characters and password at least 8 characters.");
    }
    if (role == Role::HeadOfDepartment && departmentId <= 0) {
        throw std::invalid_argument("A Head of Department account must be assigned to a department.");
    }
    Statement insert(impl_->connection,
                     "INSERT INTO users(username,password_hash,role,active,department_id) VALUES(?,?,?,1,?);");
    insert.text(1, username);
    insert.text(2, PasswordHasher::hash(password));
    insert.text(3, roleToText(role));
    if (departmentId < 0) {
        insert.null(4);
    } else insert.integer(4, departmentId);
    checkDone(insert, impl_->connection);
    return static_cast<int>(sqlite3_last_insert_rowid(impl_->connection));
}

void Database::updateUser(const User& actor, int userId, const std::string& username,
                          Role role, int departmentId, bool active, const std::string& newPassword) {
    requirePermission(impl_->connection, actor, Permission::ManageUsers);
    requireText(username, "Username");
    if (userId <= 0 || username.size() > 64 || (!newPassword.empty() && newPassword.size() < 8)) {
        throw std::invalid_argument("Invalid user ID, username, or password.");
    }
    if (role == Role::HeadOfDepartment && departmentId <= 0) {
        throw std::invalid_argument("A Head of Department account must be assigned to a department.");
    }
    if (newPassword.empty()) {
        Statement update(impl_->connection, "UPDATE users SET username=?,role=?,department_id=?,active=? WHERE user_id=?;");
        update.text(1, username);
        update.text(2, roleToText(role));
        if (departmentId < 0) update.null(3);
        else update.integer(3, departmentId);
        update.integer(4, active ? 1 : 0);
        update.integer(5, userId);
        checkDone(update, impl_->connection);
    } else {
        Statement update(impl_->connection, "UPDATE users SET username=?,role=?,department_id=?,active=?,password_hash=? WHERE user_id=?;");
        update.text(1, username);
        update.text(2, roleToText(role));
        if (departmentId < 0) update.null(3);
        else update.integer(3, departmentId);
        update.integer(4, active ? 1 : 0);
        update.text(5, PasswordHasher::hash(newPassword));
        update.integer(6, userId);
        checkDone(update, impl_->connection);
    }
    if (sqlite3_changes(impl_->connection) == 0) {
        throw std::runtime_error("User not found.");
    }
}

std::vector<UserRecord> Database::listUsers(const User& actor) {
    requirePermission(impl_->connection, actor, Permission::ManageUsers);
    Statement query(impl_->connection, "SELECT user_id,username,role,active,COALESCE(department_id,-1) FROM users ORDER BY user_id;");
    std::vector<UserRecord> records;
    while (query.step() == SQLITE_ROW) {
        records.push_back({query.intColumn(0), query.stringColumn(1), query.stringColumn(2),
                           query.intColumn(3), query.intColumn(4)});
    }
    return records;
}

int Database::createDepartment(const User& actor, const std::string& name) {
    requirePermission(impl_->connection, actor, Permission::ManageCourses);
    requireText(name, "Department name");
    Statement insert(impl_->connection, "INSERT INTO departments(name) VALUES(?);");
    insert.text(1, name);
    checkDone(insert, impl_->connection);
    return static_cast<int>(sqlite3_last_insert_rowid(impl_->connection));
}

void Database::updateDepartment(const User& actor, int departmentId, const std::string& name) {
    requirePermission(impl_->connection, actor, Permission::ManageCourses);
    requireText(name, "Department name");
    Statement update(impl_->connection, "UPDATE departments SET name=? WHERE department_id=?;");
    update.text(1, name);
    update.integer(2, departmentId);
    checkDone(update, impl_->connection);
    if (sqlite3_changes(impl_->connection) == 0) throw std::runtime_error("Department not found.");
}

std::vector<DepartmentRecord> Database::listDepartments(const User& actor) {
    requireCurrentIdentity(impl_->connection, actor);
    if (!actor.can(Permission::ManageUsers) && !actor.can(Permission::ManageCourses)) {
        throw std::runtime_error("Access denied for this role.");
    }
    Statement query(impl_->connection, "SELECT department_id,name FROM departments ORDER BY name;");
    std::vector<DepartmentRecord> records;
    while (query.step() == SQLITE_ROW) records.push_back({query.intColumn(0), query.stringColumn(1)});
    return records;
}

int Database::createCourse(const User& actor, const std::string& code, const std::string& title, int departmentId) {
    requirePermission(impl_->connection, actor, Permission::ManageCourses);
    requireText(code, "Course code");
    requireText(title, "Course title");
    if (departmentId <= 0) throw std::invalid_argument("Select a valid department.");
    Statement insert(impl_->connection, "INSERT INTO courses(code,title,department_id) VALUES(?,?,?);");
    insert.text(1, code);
    insert.text(2, title);
    insert.integer(3, departmentId);
    checkDone(insert, impl_->connection);
    return static_cast<int>(sqlite3_last_insert_rowid(impl_->connection));
}

void Database::updateCourse(const User& actor, int courseId, const std::string& code,
                            const std::string& title, int departmentId) {
    requirePermission(impl_->connection, actor, Permission::ManageCourses);
    requireText(code, "Course code");
    requireText(title, "Course title");
    Statement update(impl_->connection, "UPDATE courses SET code=?,title=?,department_id=? WHERE course_id=?;");
    update.text(1, code);
    update.text(2, title);
    update.integer(3, departmentId);
    update.integer(4, courseId);
    checkDone(update, impl_->connection);
    if (sqlite3_changes(impl_->connection) == 0) throw std::runtime_error("Course not found.");
}

void Database::assignLecturer(const User& actor, int lecturerId, int courseId) {
    requirePermission(impl_->connection, actor, Permission::ManageCourses);
    Statement insert(impl_->connection, "INSERT OR IGNORE INTO lecturer_courses(lecturer_id,course_id) "
                                         "SELECT user_id,? FROM users WHERE user_id=? AND role='LECTURER' AND active=1;");
    insert.integer(1, courseId);
    insert.integer(2, lecturerId);
    checkDone(insert, impl_->connection);
    if (sqlite3_changes(impl_->connection) == 0) throw std::runtime_error("Active lecturer or course not found, or assignment already exists.");
}

void Database::enrolStudent(const User& actor, int studentId, int courseId) {
    requirePermission(impl_->connection, actor, Permission::ManageCourses);
    Statement insert(impl_->connection, "INSERT OR IGNORE INTO enrolments(student_id,course_id) "
                                         "SELECT user_id,? FROM users WHERE user_id=? AND role='STUDENT' AND active=1;");
    insert.integer(1, courseId);
    insert.integer(2, studentId);
    checkDone(insert, impl_->connection);
    if (sqlite3_changes(impl_->connection) == 0) throw std::runtime_error("Active student or course not found, or enrolment already exists.");
}

std::vector<CourseRecord> Database::coursesFor(const User& user) {
    requireCurrentIdentity(impl_->connection, user);
    std::string sql =
        "SELECT c.course_id,c.code,c.title,c.department_id,d.name FROM courses c "
        "JOIN departments d ON d.department_id=c.department_id ";
    if (user.role() == Role::Lecturer) sql += "JOIN lecturer_courses lc ON lc.course_id=c.course_id WHERE lc.lecturer_id=? ";
    else if (user.role() == Role::Student) sql += "JOIN enrolments e ON e.course_id=c.course_id WHERE e.student_id=? ";
    else if (user.role() == Role::HeadOfDepartment) sql += "WHERE c.department_id=? ";
    sql += "ORDER BY c.code;";
    Statement query(impl_->connection, sql);
    if (user.role() != Role::Administrator) query.integer(1, user.role() == Role::HeadOfDepartment ? user.departmentId() : user.id());
    std::vector<CourseRecord> records;
    while (query.step() == SQLITE_ROW) {
        records.push_back({query.intColumn(0), query.stringColumn(1), query.stringColumn(2),
                           query.intColumn(3), query.stringColumn(4)});
    }
    return records;
}

std::vector<AssignmentRecord> Database::assignmentsForLecturer(const User& lecturer) {
    requirePermission(impl_->connection, lecturer, Permission::TeachCourse);
    Statement query(impl_->connection, "SELECT a.assignment_id,a.course_id,c.code,a.title,a.deadline,a.deadline,a.max_mark,"
                                       "a.attachment_path,COUNT(s.submission_id),"
                                       "SUM(CASE WHEN s.status IN ('MARKED','MODERATED','RELEASED') THEN 1 ELSE 0 END) "
                                       "FROM assignments a JOIN courses c ON c.course_id=a.course_id "
                                       "JOIN lecturer_courses lc ON lc.course_id=c.course_id "
                                       "LEFT JOIN submissions s ON s.assignment_id=a.assignment_id AND s.is_active=1 "
                                       "WHERE lc.lecturer_id=? GROUP BY a.assignment_id ORDER BY a.deadline;");
    query.integer(1, lecturer.id());
    std::vector<AssignmentRecord> records;
    while (query.step() == SQLITE_ROW) {
        records.push_back({query.intColumn(0), query.intColumn(1), query.stringColumn(2), query.stringColumn(3),
                           query.stringColumn(4), query.stringColumn(5), query.realColumn(6),
                           query.stringColumn(7), query.intColumn(8), query.intColumn(9)});
    }
    return records;
}

std::vector<AssignmentRecord> Database::assignmentsForDepartment(const User& hod) {
    requirePermission(impl_->connection, hod, Permission::ModerateDepartment);
    Statement query(impl_->connection,
        "SELECT a.assignment_id,a.course_id,c.code,a.title,a.deadline,a.deadline,a.max_mark,a.attachment_path,"
        "COUNT(s.submission_id),SUM(CASE WHEN s.status IN ('MARKED','MODERATED','RELEASED') THEN 1 ELSE 0 END) "
        "FROM assignments a JOIN courses c ON c.course_id=a.course_id "
        "LEFT JOIN submissions s ON s.assignment_id=a.assignment_id AND s.is_active=1 "
        "WHERE c.department_id=? GROUP BY a.assignment_id ORDER BY a.deadline,c.code;");
    query.integer(1, hod.departmentId());
    std::vector<AssignmentRecord> records;
    while (query.step() == SQLITE_ROW) {
        records.push_back({query.intColumn(0), query.intColumn(1), query.stringColumn(2), query.stringColumn(3),
                           query.stringColumn(4), query.stringColumn(5), query.realColumn(6),
                           query.stringColumn(7), query.intColumn(8), query.intColumn(9)});
    }
    return records;
}

int Database::createAssignment(const User& actor, int courseId, const std::string& title,
                               const std::string& deadline, double maxMark,
                               const std::string& attachmentPath) {
    requirePermission(impl_->connection, actor, Permission::TeachCourse);
    requireText(title, "Assignment title");
    if (maxMark <= 0.0 || !std::isfinite(maxMark)) throw std::invalid_argument("Maximum mark must be a positive number.");
    const std::string normalizedDeadline = normalizeDateTime(deadline);
    Statement insert(impl_->connection, "INSERT INTO assignments(course_id,title,deadline,max_mark,attachment_path) "
                                         "SELECT ?,?,?,?,? WHERE EXISTS "
                                         "(SELECT 1 FROM lecturer_courses WHERE lecturer_id=? AND course_id=?);");
    insert.integer(1, courseId);
    insert.text(2, title);
    insert.text(3, normalizedDeadline);
    insert.real(4, maxMark);
    insert.text(5, attachmentPath);
    insert.integer(6, actor.id());
    insert.integer(7, courseId);
    checkDone(insert, impl_->connection);
    if (sqlite3_changes(impl_->connection) == 0) throw std::runtime_error("You are not assigned to that course.");
    return static_cast<int>(sqlite3_last_insert_rowid(impl_->connection));
}

void Database::grantExtension(const User& actor, int assignmentId, int studentId,
                              const std::string& newDeadline, const std::string& reason) {
    requirePermission(impl_->connection, actor, Permission::TeachCourse);
    requireText(reason, "Extension reason");
    const std::string normalizedDeadline = normalizeDateTime(newDeadline);
    Transaction transaction(impl_->connection);
    Statement existing(impl_->connection, "SELECT a.deadline FROM assignments a JOIN lecturer_courses lc ON lc.course_id=a.course_id "
                                          "JOIN enrolments e ON e.course_id=a.course_id WHERE a.assignment_id=? "
                                          "AND lc.lecturer_id=? AND e.student_id=?;");
    existing.integer(1, assignmentId);
    existing.integer(2, actor.id());
    existing.integer(3, studentId);
    if (existing.step() != SQLITE_ROW) throw std::runtime_error("Assignment, student enrolment, or lecturer assignment not found.");
    if (toTimestamp(normalizedDeadline) <= toTimestamp(existing.stringColumn(0))) {
        throw std::invalid_argument("An extension must be later than the original deadline.");
    }
    Statement current(impl_->connection, "SELECT status FROM submissions WHERE assignment_id=? AND student_id=? "
                                         "AND is_active=1;");
    current.integer(1, assignmentId);
    current.integer(2, studentId);
    if (current.step() == SQLITE_ROW && current.stringColumn(0) != "SUBMITTED" &&
        current.stringColumn(0) != "LATE") {
        throw std::runtime_error("An extension cannot change an already marked result.");
    }
    Statement upsert(impl_->connection, "INSERT INTO extensions(assignment_id,student_id,new_deadline,reason,granted_by) "
                                        "VALUES(?,?,?,?,?) ON CONFLICT(assignment_id,student_id) DO UPDATE SET "
                                        "new_deadline=excluded.new_deadline,reason=excluded.reason,granted_by=excluded.granted_by;");
    upsert.integer(1, assignmentId);
    upsert.integer(2, studentId);
    upsert.text(3, normalizedDeadline);
    upsert.text(4, reason);
    upsert.integer(5, actor.id());
    checkDone(upsert, impl_->connection);
    Statement refreshStatus(impl_->connection, "UPDATE submissions SET status=CASE WHEN submitted_at<=? "
                                               "THEN 'SUBMITTED' ELSE 'LATE' END WHERE assignment_id=? "
                                               "AND student_id=? AND is_active=1 AND status IN ('SUBMITTED','LATE');");
    refreshStatus.text(1, normalizedDeadline);
    refreshStatus.integer(2, assignmentId);
    refreshStatus.integer(3, studentId);
    checkDone(refreshStatus, impl_->connection);
    transaction.commit();
}

std::vector<AssignmentRecord> Database::assignmentsForStudent(const User& student) {
    requirePermission(impl_->connection, student, Permission::SubmitWork);
    Statement query(impl_->connection, "SELECT a.assignment_id,a.course_id,c.code,a.title,a.deadline,"
                                       "COALESCE(e.new_deadline,a.deadline),a.max_mark,a.attachment_path FROM assignments a "
                                       "JOIN courses c ON c.course_id=a.course_id JOIN enrolments en ON en.course_id=a.course_id "
                                       "LEFT JOIN extensions e ON e.assignment_id=a.assignment_id AND e.student_id=en.student_id "
                                       "WHERE en.student_id=? ORDER BY a.deadline;");
    query.integer(1, student.id());
    std::vector<AssignmentRecord> records;
    while (query.step() == SQLITE_ROW) {
        records.push_back({query.intColumn(0), query.intColumn(1), query.stringColumn(2), query.stringColumn(3),
                           query.stringColumn(4), query.stringColumn(5), query.realColumn(6),
                           query.stringColumn(7), 0, 0});
    }
    return records;
}

SubmissionReceipt Database::submit(const User& student, int assignmentId, const std::string& filePath) {
    requirePermission(impl_->connection, student, Permission::SubmitWork);
    requireText(filePath, "Submission entry or file path");
    const std::string submittedAt = currentDateTime();
    sqlite3* db = impl_->connection;
    Transaction transaction(db);
    Statement assignment(db, "SELECT COALESCE(e.new_deadline,a.deadline),a.max_mark FROM assignments a "
                             "JOIN enrolments en ON en.course_id=a.course_id AND en.student_id=? "
                             "LEFT JOIN extensions e ON e.assignment_id=a.assignment_id AND e.student_id=en.student_id "
                             "WHERE a.assignment_id=?;");
    assignment.integer(1, student.id());
    assignment.integer(2, assignmentId);
    if (assignment.step() != SQLITE_ROW) throw std::runtime_error("Assignment not found or you are not enrolled.");
    const std::string effectiveDeadline = assignment.stringColumn(0);
    const double maximumMark = assignment.realColumn(1);
    Statement current(db, "SELECT submission_id,version_no,status FROM submissions WHERE assignment_id=? "
                          "AND student_id=? AND is_active=1;");
    current.integer(1, assignmentId);
    current.integer(2, student.id());
    int version = 1;
    if (current.step() == SQLITE_ROW) {
        if (toTimestamp(submittedAt) > toTimestamp(effectiveDeadline)) {
            throw std::runtime_error("Resubmissions are only allowed before the effective deadline.");
        }
        const std::string status = current.stringColumn(2);
        if (status == "MARKED" || status == "MODERATED" || status == "RELEASED") {
            throw std::runtime_error("A marked submission cannot be replaced.");
        }
        version = current.intColumn(1) + 1;
        Statement deactivate(db, "UPDATE submissions SET is_active=0 WHERE submission_id=?;");
        deactivate.integer(1, current.intColumn(0));
        checkDone(deactivate, db);
    }
    const int lateDayCount = daysLate(submittedAt, effectiveDeadline);
    const std::string status = lateDayCount == 0 ? "SUBMITTED" : "LATE";
    Statement insert(db, "INSERT INTO submissions(assignment_id,student_id,version_no,submitted_at,status,file_path,is_active) "
                         "VALUES(?,?,?,?,?,?,1);");
    insert.integer(1, assignmentId);
    insert.integer(2, student.id());
    insert.integer(3, version);
    insert.text(4, submittedAt);
    insert.text(5, status);
    insert.text(6, filePath);
    checkDone(insert, db);
    const int id = static_cast<int>(sqlite3_last_insert_rowid(db));
    transaction.commit();
    StandardLatePenalty strategy;
    return {id, version, status, lateDayCount,
            strategy.penalty(maximumMark, lateDayCount), lateDayCount > 3, filePath};
}

std::vector<SubmissionRecord> Database::submissionsForStudent(const User& student) {
    requirePermission(impl_->connection, student, Permission::SubmitWork);
    std::string sql = submissionSelect;
    sql += "LEFT JOIN marks m ON m.submission_id=s.submission_id "
           "WHERE s.student_id=? ORDER BY a.title,s.version_no;";
    Statement query(impl_->connection, sql);
    query.integer(1, student.id());
    std::vector<SubmissionRecord> records;
    while (query.step() == SQLITE_ROW) records.push_back(readSubmission(query));
    return records;
}

std::vector<SubmissionRecord> Database::submissionsForLecturer(const User& lecturer) {
    requirePermission(impl_->connection, lecturer, Permission::TeachCourse);
    std::string sql = submissionSelect;
    sql += "LEFT JOIN marks m ON m.submission_id=s.submission_id JOIN lecturer_courses lc ON lc.course_id=a.course_id "
           "WHERE lc.lecturer_id=? AND s.is_active=1 AND s.status IN ('SUBMITTED','LATE','MARKED') "
           "ORDER BY a.deadline,c.code,u.username;";
    Statement query(impl_->connection, sql);
    query.integer(1, lecturer.id());
    std::vector<SubmissionRecord> records;
    while (query.step() == SQLITE_ROW) records.push_back(readSubmission(query));
    return records;
}

void Database::saveMark(const User& lecturer, int submissionId, double rawMark,
                        const std::string& feedback, const std::string& reason) {
    requirePermission(impl_->connection, lecturer, Permission::TeachCourse);
    requireText(feedback, "Feedback");
    requireText(reason, "Mark change reason");
    if (!std::isfinite(rawMark) || rawMark < 0.0) throw std::invalid_argument("Mark must be zero or greater.");
    sqlite3* db = impl_->connection;
    Transaction transaction(db);
    Statement submission(db, "SELECT a.max_mark,s.submitted_at,COALESCE(e.new_deadline,a.deadline),s.status "
                             "FROM submissions s JOIN assignments a ON a.assignment_id=s.assignment_id "
                             "JOIN lecturer_courses lc ON lc.course_id=a.course_id AND lc.lecturer_id=? "
                             "LEFT JOIN extensions e ON e.assignment_id=a.assignment_id AND e.student_id=s.student_id "
                             "WHERE s.submission_id=? AND s.is_active=1;");
    submission.integer(1, lecturer.id());
    submission.integer(2, submissionId);
    if (submission.step() != SQLITE_ROW) throw std::runtime_error("Submission not found or not assigned to you.");
    const double maximum = submission.realColumn(0);
    const std::string status = submission.stringColumn(3);
    if (rawMark > maximum) throw std::invalid_argument("Mark cannot exceed the assignment maximum.");
    if (status != "SUBMITTED" && status != "LATE" && status != "MARKED") {
        throw std::runtime_error("This submission cannot be marked in its current state.");
    }
    const int lateDays = daysLate(submission.stringColumn(1), submission.stringColumn(2));
    StandardLatePenalty strategy;
    const double penalty = strategy.penalty(maximum, lateDays);
    const double finalMark = strategy.finalMark(rawMark, maximum, lateDays);
    Statement prior(db, "SELECT final_mark FROM marks WHERE submission_id=?;");
    prior.integer(1, submissionId);
    const bool existed = prior.step() == SQLITE_ROW;
    const double oldMark = existed ? prior.realColumn(0) : 0.0;
    const std::string auditReason = existed ? reason : "Initial mark";
    Statement upsert(db, "INSERT INTO marks(submission_id,raw_mark,penalty,final_mark,feedback,marked_by) "
                         "VALUES(?,?,?,?,?,?) ON CONFLICT(submission_id) DO UPDATE SET raw_mark=excluded.raw_mark,"
                         "penalty=excluded.penalty,final_mark=excluded.final_mark,feedback=excluded.feedback,marked_by=excluded.marked_by;");
    upsert.integer(1, submissionId);
    upsert.real(2, rawMark);
    upsert.real(3, penalty);
    upsert.real(4, finalMark);
    upsert.text(5, feedback);
    bindActorId(upsert, 6, lecturer);
    checkDone(upsert, db);
    Statement audit(db, "INSERT INTO mark_audit(submission_id,changed_by,old_mark,new_mark,reason,changed_at) "
                        "VALUES(?,?,?,?,?,?);");
    audit.integer(1, submissionId);
    bindActorId(audit, 2, lecturer);
    if (existed) audit.real(3, oldMark);
    else audit.null(3);
    audit.real(4, finalMark);
    audit.text(5, auditReason);
    audit.text(6, currentDateTime());
    checkDone(audit, db);
    Statement state(db, "UPDATE submissions SET status='MARKED' WHERE submission_id=?;");
    state.integer(1, submissionId);
    checkDone(state, db);
    transaction.commit();
}

std::vector<SubmissionRecord> Database::submissionsForModeration(const User& hod) {
    requirePermission(impl_->connection, hod, Permission::ModerateDepartment);
    std::string sql = submissionSelect;
    sql += "JOIN marks m ON m.submission_id=s.submission_id JOIN departments d ON d.department_id=c.department_id "
           "WHERE d.department_id=? AND s.is_active=1 AND s.status='MARKED' ORDER BY c.code,u.username;";
    Statement query(impl_->connection, sql);
    query.integer(1, hod.departmentId());
    std::vector<SubmissionRecord> records;
    while (query.step() == SQLITE_ROW) records.push_back(readSubmission(query));
    return records;
}

std::vector<SubmissionRecord> Database::submissionsReadyForRelease(const User& hod) {
    requirePermission(impl_->connection, hod, Permission::ModerateDepartment);
    std::string sql = submissionSelect;
    sql += "JOIN marks m ON m.submission_id=s.submission_id "
           "WHERE c.department_id=? AND s.is_active=1 AND s.status='MODERATED' ORDER BY c.code,u.username;";
    Statement query(impl_->connection, sql);
    query.integer(1, hod.departmentId());
    std::vector<SubmissionRecord> records;
    while (query.step() == SQLITE_ROW) records.push_back(readSubmission(query));
    return records;
}

void Database::moderateSubmission(const User& hod, int submissionId,
                                  const std::optional<double>& revisedMark, const std::string& reason) {
    requirePermission(impl_->connection, hod, Permission::ModerateDepartment);
    if (revisedMark && (!std::isfinite(*revisedMark) || *revisedMark < 0.0)) {
        throw std::invalid_argument("Revised mark must be zero or greater.");
    }
    if (revisedMark) requireText(reason, "Moderation reason");
    sqlite3* db = impl_->connection;
    Transaction transaction(db);
    Statement existing(db, "SELECT m.final_mark,a.max_mark FROM marks m JOIN submissions s ON s.submission_id=m.submission_id "
                           "JOIN assignments a ON a.assignment_id=s.assignment_id JOIN courses c ON c.course_id=a.course_id "
                           "WHERE s.submission_id=? AND s.status='MARKED' AND c.department_id=? AND s.is_active=1;");
    existing.integer(1, submissionId);
    existing.integer(2, hod.departmentId());
    if (existing.step() != SQLITE_ROW) throw std::runtime_error("Marked submission not found in your department.");
    const double oldMark = existing.realColumn(0);
    const double maximum = existing.realColumn(1);
    if (revisedMark) {
        if (*revisedMark > maximum) throw std::invalid_argument("Revised mark cannot exceed the assignment maximum.");
        Statement update(db, "UPDATE marks SET final_mark=? WHERE submission_id=?;");
        update.real(1, *revisedMark);
        update.integer(2, submissionId);
        checkDone(update, db);
        Statement audit(db, "INSERT INTO mark_audit(submission_id,changed_by,old_mark,new_mark,reason,changed_at) "
                            "VALUES(?,?,?,?,?,?);");
        audit.integer(1, submissionId);
        audit.integer(2, hod.id());
        audit.real(3, oldMark);
        audit.real(4, *revisedMark);
        audit.text(5, reason);
        audit.text(6, currentDateTime());
        checkDone(audit, db);
    }
    Statement moderate(db, "UPDATE submissions SET status='MODERATED' WHERE submission_id=? AND status='MARKED';");
    moderate.integer(1, submissionId);
    checkDone(moderate, db);
    transaction.commit();
}

void Database::releaseModerated(const User& hod, int submissionId) {
    requirePermission(impl_->connection, hod, Permission::ModerateDepartment);
    Statement release(impl_->connection,
        "UPDATE submissions SET status='RELEASED' WHERE submission_id=? AND status='MODERATED' "
        "AND EXISTS (SELECT 1 FROM assignments a JOIN courses c ON c.course_id=a.course_id "
        "WHERE a.assignment_id=submissions.assignment_id AND c.department_id=?);");
    release.integer(1, submissionId);
    release.integer(2, hod.departmentId());
    checkDone(release, impl_->connection);
    if (sqlite3_changes(impl_->connection) == 0) {
        throw std::runtime_error("Moderated submission not found in your department.");
    }
}

std::vector<SubmissionRecord> Database::releasedMarks(const User& student) {
    requirePermission(impl_->connection, student, Permission::ViewReleasedMarks);
    std::string sql = submissionSelect;
    sql += "JOIN marks m ON m.submission_id=s.submission_id WHERE s.student_id=? AND s.status='RELEASED' "
           "AND s.is_active=1 ORDER BY c.code,a.title;";
    Statement query(impl_->connection, sql);
    query.integer(1, student.id());
    std::vector<SubmissionRecord> records;
    while (query.step() == SQLITE_ROW) records.push_back(readSubmission(query));
    return records;
}

std::vector<AuditRecord> Database::auditForDepartment(const User& hod) {
    requirePermission(impl_->connection, hod, Permission::ModerateDepartment);
    Statement query(impl_->connection,
        "SELECT ma.audit_id,a.title || ' / ' || c.code,u.username,ma.old_mark,ma.new_mark,ma.reason,ma.changed_at "
        "FROM mark_audit ma JOIN submissions s ON s.submission_id=ma.submission_id "
        "JOIN assignments a ON a.assignment_id=s.assignment_id JOIN courses c ON c.course_id=a.course_id "
        "JOIN users u ON u.user_id=ma.changed_by WHERE c.department_id=? ORDER BY ma.audit_id;");
    query.integer(1, hod.departmentId());
    std::vector<AuditRecord> records;
    while (query.step() == SQLITE_ROW) {
        AuditRecord record{query.intColumn(0), query.stringColumn(1), query.stringColumn(2),
                           std::nullopt, query.realColumn(4), query.stringColumn(5), query.stringColumn(6)};
        if (!query.isNull(3)) record.oldMark = query.realColumn(3);
        records.push_back(std::move(record));
    }
    return records;
}
