#include "Database.h"
#include "FileStorage.h"

#include <cctype>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string value;
    if (!std::getline(std::cin, value)) throw std::runtime_error("Input ended.");
    if (!value.empty() && value.back() == '\r') value.pop_back();
    return value;
}

int readInt(const std::string& prompt) {
    for (;;) {
        const std::string value = readLine(prompt);
        std::size_t consumed = 0;
        try {
            const int result = std::stoi(value, &consumed);
            if (consumed == value.size()) return result;
        } catch (const std::exception&) {
        }
        std::cout << "Enter a valid whole number.\n";
    }
}

double readDouble(const std::string& prompt) {
    const std::string value = readLine(prompt);
    std::size_t consumed = 0;
    try {
        const double result = std::stod(value, &consumed);
        if (consumed != value.size()) throw std::invalid_argument("trailing characters");
        return result;
    } catch (const std::exception&) {
        throw std::invalid_argument("Enter a valid number.");
    }
}

void printHeader(const std::string& value) {
    std::cout << "\n=== " << value << " ===\n";
}

Role readRole() {
    std::string value = readLine("Role (ADMIN/LECTURER/HOD/STUDENT): ");
    for (char& character : value) {
        character = static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
    }
    if (value == "ADMIN") return Role::Administrator;
    if (value == "LECTURER") return Role::Lecturer;
    if (value == "HOD") return Role::HeadOfDepartment;
    if (value == "STUDENT") return Role::Student;
    throw std::invalid_argument("Role must be ADMIN, LECTURER, HOD, or STUDENT.");
}

bool confirm(const std::string& prompt) {
    for (;;) {
        std::string answer = readLine(prompt + " (y/n): ");
        for (char& character : answer) {
            character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
        }
        if (answer == "y" || answer == "yes") return true;
        if (answer == "n" || answer == "no") return false;
        std::cout << "Enter y or n.\n";
    }
}

void printDepartments(Database& database, const User& user) {
    std::cout << "Departments:\n";
    for (const auto& department : database.listDepartments(user)) {
        std::cout << "  " << department.id << " - " << department.name << '\n';
    }
}

void printCourses(Database& database, const User& user) {
    std::cout << "Courses:\n";
    const auto courses = database.coursesFor(user);
    if (courses.empty()) std::cout << "  (none)\n";
    for (const auto& course : courses) {
        std::cout << "  " << course.id << " - " << course.code << ": " << course.title
                  << " [" << course.departmentName << "]\n";
    }
}

void printAssignments(const std::vector<AssignmentRecord>& assignments, bool showCounts = false) {
    if (assignments.empty()) {
        std::cout << "No assignments found.\n";
        return;
    }
    for (const auto& assignment : assignments) {
        std::cout << "  ID " << assignment.id << " | " << assignment.courseCode << " | "
                  << assignment.title << " | deadline " << assignment.deadline
                  << " | effective " << assignment.effectiveDeadline
                  << " | max " << assignment.maxMark;
        if (showCounts) {
            std::cout << " | active submissions " << assignment.submissionCount
                      << " | marked/released " << assignment.markedCount;
        }
        std::cout << '\n';
        if (!assignment.attachmentPath.empty()) {
            std::cout << "    Assignment document: " << assignment.attachmentPath << '\n';
        }
    }
}

void printSubmissions(const std::vector<SubmissionRecord>& submissions) {
    if (submissions.empty()) {
        std::cout << "No submissions found.\n";
        return;
    }
    for (const auto& submission : submissions) {
        std::cout << "  ID " << submission.id << " | " << submission.courseCode << " | "
                  << submission.assignmentTitle << " | student " << submission.studentName
                  << " | v" << submission.version << " | " << submission.status
                  << " | submitted " << submission.submittedAt;
        std::cout << "\n    Student document: " << submission.filePath;
        if (!submission.assignmentAttachmentPath.empty()) {
            std::cout << "\n    Assignment brief: " << submission.assignmentAttachmentPath;
        }
        if (submission.status == "MARKED" || submission.status == "MODERATED") {
            std::cout << "\n    final " << submission.finalMark << "/" << submission.maxMark
                      << " | feedback: " << submission.feedback;
        }
        std::cout << '\n';
    }
}

void printStudentSubmissions(const std::vector<SubmissionRecord>& submissions) {
    if (submissions.empty()) {
        std::cout << "You have not submitted any work yet.\n";
        return;
    }
    bool awaitingRelease = false;
    for (const auto& submission : submissions) {
        std::cout << "  ID " << submission.id << " | " << submission.courseCode << " | "
                  << submission.assignmentTitle << " | v" << submission.version
                  << " | " << submission.status << " | submitted " << submission.submittedAt
                  << "\n    Your document: " << submission.filePath << '\n';
        if (submission.status == "RELEASED") {
            std::cout << "    final " << submission.finalMark << "/" << submission.maxMark
                      << " | feedback: " << submission.feedback << '\n';
        } else if (submission.status == "MARKED" || submission.status == "MODERATED") {
            awaitingRelease = true;
        }
    }
    if (awaitingRelease) {
        std::cout << "Marks for work still marked or moderated are hidden until the HoD releases them "
                     "using option 4.\n";
    }
}

const AssignmentRecord& findAssignment(const std::vector<AssignmentRecord>& assignments, int id) {
    for (const auto& assignment : assignments) {
        if (assignment.id == id) return assignment;
    }
    throw std::invalid_argument("Choose an assignment ID from the list.");
}

const SubmissionRecord& findSubmission(const std::vector<SubmissionRecord>& submissions, int id) {
    for (const auto& submission : submissions) {
        if (submission.id == id) return submission;
    }
    throw std::invalid_argument("Choose a submission ID from the list.");
}

void runAdmin(Database& database, const User& user) {
    for (;;) {
        printHeader("Administrator");
        std::cout << "1. Create user\n2. Edit/activate/deactivate user\n3. List users\n"
                     "4. Create department\n5. Create course\n6. Edit course\n"
                     "7. Assign lecturer to course\n8. Enrol student in course\n"
                     "9. List courses\n10. Edit department\n0. Log out\n";
        const int choice = readInt("Choice: ");
        if (choice == 0) return;
        try {
            if (choice == 1) {
                const std::string username = readLine("Username: ");
                const std::string password = readLine("Password (at least 8 characters): ");
                const Role role = readRole();
                printDepartments(database, user);
                const int departmentId = readInt("Department ID (0 for none): ");
                database.createUser(user, username, password, role, departmentId == 0 ? -1 : departmentId);
                std::cout << "User created.\n";
            } else if (choice == 2) {
                for (const auto& record : database.listUsers(user)) {
                    std::cout << "  " << record.id << " | " << record.username << " | " << record.role
                              << " | " << (record.active ? "active" : "disabled")
                              << " | department " << record.departmentId << '\n';
                }
                const int id = readInt("User ID: ");
                const auto records = database.listUsers(user);
                std::vector<UserRecord>::const_iterator current = records.begin();
                while (current != records.end() && current->id != id) ++current;
                if (current == records.end()) throw std::runtime_error("User not found.");
                const std::string username = readLine("New username: ");
                const Role role = readRole();
                printDepartments(database, user);
                const int departmentId = readInt("Department ID (0 for none): ");
                std::string active;
                do {
                    active = readLine("Active? (y/n): ");
                    if (active != "y" && active != "Y" && active != "n" && active != "N") {
                        std::cout << "Enter y or n.\n";
                    }
                } while (active != "y" && active != "Y" && active != "n" && active != "N");
                const std::string password = readLine("New password (blank to keep current): ");
                database.updateUser(user, id, username, role, departmentId == 0 ? -1 : departmentId,
                                    active == "y" || active == "Y", password);
                std::cout << "User updated.\n";
            } else if (choice == 3) {
                for (const auto& record : database.listUsers(user)) {
                    std::cout << record.id << " | " << record.username << " | " << record.role
                              << " | " << (record.active ? "active" : "disabled") << '\n';
                }
            } else if (choice == 4) {
                const int id = database.createDepartment(user, readLine("Department name: "));
                std::cout << "Department created with ID " << id << ".\n";
            } else if (choice == 5) {
                printDepartments(database, user);
                const std::string code = readLine("Course code: ");
                const std::string title = readLine("Course title: ");
                const int departmentId = readInt("Department ID: ");
                const int id = database.createCourse(user, code, title, departmentId);
                std::cout << "Course created with ID " << id << ".\n";
            } else if (choice == 6) {
                printCourses(database, user);
                database.updateCourse(user, readInt("Course ID: "), readLine("Course code: "),
                                      readLine("Course title: "), readInt("Department ID: "));
                std::cout << "Course updated.\n";
            } else if (choice == 7) {
                printCourses(database, user);
                const int lecturerId = readInt("Lecturer user ID: ");
                const int courseId = readInt("Course ID: ");
                database.assignLecturer(user, lecturerId, courseId);
                std::cout << "Lecturer assigned.\n";
            } else if (choice == 8) {
                printCourses(database, user);
                const int studentId = readInt("Student user ID: ");
                const int courseId = readInt("Course ID: ");
                database.enrolStudent(user, studentId, courseId);
                std::cout << "Student enrolled.\n";
            } else if (choice == 9) {
                printCourses(database, user);
            } else if (choice == 10) {
                printDepartments(database, user);
                database.updateDepartment(user, readInt("Department ID: "), readLine("New department name: "));
                std::cout << "Department updated.\n";
            } else {
                std::cout << "Choose an option from 0 to 10.\n";
            }
        } catch (const std::exception& error) {
            std::cerr << "Action failed: " << error.what() << '\n';
        }
    }
}

void runLecturer(Database& database, const User& user, const FileStorage& files) {
    for (;;) {
        printHeader("Lecturer");
        std::cout << "1. List assigned courses\n2. List assignments\n3. Create assignment\n"
                     "4. Grant/update extension\n5. View and mark submissions\n"
                     "6. Open an assignment brief\n0. Log out\n";
        const int choice = readInt("Choice: ");
        if (choice == 0) return;
        try {
            if (choice == 1) {
                printCourses(database, user);
            } else if (choice == 2) {
                printAssignments(database.assignmentsForLecturer(user), true);
            } else if (choice == 3) {
                printCourses(database, user);
                const int courseId = readInt("Course ID: ");
                const std::string title = readLine("Assignment title: ");
                const std::string deadline = readLine("Deadline (YYYY-MM-DD HH:MM): ");
                const double maximum = readDouble("Maximum mark: ");
                const std::string source = readLine(
                    "Assignment brief document path (PDF/DOCX/ODT/RTF/TXT/PPTX/XLSX/CSV; blank to skip): ");
                std::string storedPath;
                if (!source.empty()) storedPath = files.storeDocument(source, UploadKind::Assignment);
                try {
                    std::cout << "Assignment created with ID "
                              << database.createAssignment(user, courseId, title, deadline, maximum, storedPath)
                              << ".\n";
                } catch (const std::exception& error) {
                    if (!storedPath.empty()) {
                        try {
                            files.removeStoredFile(storedPath);
                        } catch (const std::exception& cleanupError) {
                            throw std::runtime_error(std::string(error.what()) +
                                "; could not clean up uploaded brief: " + cleanupError.what());
                        }
                    }
                    throw;
                }
            } else if (choice == 4) {
                printCourses(database, user);
                printAssignments(database.assignmentsForLecturer(user));
                const int assignment = readInt("Assignment ID: ");
                const int student = readInt("Student user ID: ");
                const std::string deadline = readLine("Revised deadline (YYYY-MM-DD HH:MM): ");
                const std::string reason = readLine("Reason: ");
                database.grantExtension(user, assignment, student, deadline, reason);
                std::cout << "Extension saved.\n";
            } else if (choice == 5) {
                const auto submissions = database.submissionsForLecturer(user);
                printSubmissions(submissions);
                if (!submissions.empty()) {
                    const int id = readInt("Submission ID to mark/edit (0 to cancel): ");
                    if (id != 0) {
                        const SubmissionRecord& submission = findSubmission(submissions, id);
                        if (confirm("Open the student's document before marking")) {
                            files.openDocument(submission.filePath);
                            if (!submission.assignmentAttachmentPath.empty()) {
                                files.openDocument(submission.assignmentAttachmentPath);
                            }
                        }
                        const double mark = readDouble("Raw mark: ");
                        const std::string feedback = readLine("Written feedback: ");
                        std::string reason = "Initial mark";
                        if (submission.status == "MARKED") {
                            reason = readLine("Reason for mark change: ");
                        }
                        database.saveMark(user, id, mark, feedback, reason);
                        std::cout << "Mark saved. It is hidden until HoD release.\n";
                    }
                }
            } else if (choice == 6) {
                const auto assignments = database.assignmentsForLecturer(user);
                printAssignments(assignments, true);
                if (!assignments.empty()) {
                    const int id = readInt("Assignment ID to open (0 to cancel): ");
                    if (id != 0) files.openDocument(findAssignment(assignments, id).attachmentPath);
                }
            } else {
                std::cout << "Choose an option from 0 to 6.\n";
            }
        } catch (const std::exception& error) {
            std::cerr << "Action failed: " << error.what() << '\n';
        }
    }
}

void runHod(Database& database, const User& user, const FileStorage& files) {
    for (;;) {
        printHeader("Head of Department");
        std::cout << "1. List departmental courses\n2. View department assignments and submission counts\n"
                     "3. Moderate marked work (lecturer-marked, not yet moderated)\n"
                     "4. Release moderated work (after option 3)\n"
                     "5. View mark audit history\n0. Log out\n";
        const int choice = readInt("Choice: ");
        if (choice == 0) return;
        try {
            if (choice == 1) {
                printCourses(database, user);
            } else if (choice == 2) {
                printAssignments(database.assignmentsForDepartment(user), true);
            } else if (choice == 3) {
                const auto submissions = database.submissionsForModeration(user);
                printSubmissions(submissions);
                if (submissions.empty()) {
                    std::cout << "No submissions are awaiting moderation. Option 4 contains work already "
                                 "moderated; lecturer-marked work appears here in option 3.\n";
                } else {
                    const int id = readInt("Submission ID to moderate (0 to cancel): ");
                    if (id != 0) {
                        const SubmissionRecord& submission = findSubmission(submissions, id);
                        if (confirm("Open the student's document and assignment brief before moderation")) {
                            files.openDocument(submission.filePath);
                            if (!submission.assignmentAttachmentPath.empty()) {
                                files.openDocument(submission.assignmentAttachmentPath);
                            }
                        }
                        const std::string mark = readLine("New final mark (blank to approve unchanged): ");
                        std::optional<double> revised;
                        std::string reason;
                        if (!mark.empty()) {
                            std::size_t consumed = 0;
                            const double value = std::stod(mark, &consumed);
                            if (consumed != mark.size()) throw std::invalid_argument("Enter a valid mark.");
                            revised = value;
                            reason = readLine("Reason for mark change: ");
                        }
                        database.moderateSubmission(user, id, revised, reason);
                        if (confirm("Release this moderated result to the student now")) {
                            database.releaseModerated(user, id);
                            std::cout << "Moderation saved and result released to the student.\n";
                        } else {
                            std::cout << "Moderation saved. The result remains hidden until you release it "
                                         "using option 4.\n";
                        }
                    }
                }
            } else if (choice == 4) {
                const auto submissions = database.submissionsReadyForRelease(user);
                printSubmissions(submissions);
                if (submissions.empty()) {
                    std::cout << "No submissions are ready for release. First moderate a lecturer-marked "
                                 "submission using option 3.\n";
                } else {
                    const int id = readInt("Submission ID to release (0 to cancel): ");
                    if (id != 0) {
                        database.releaseModerated(user, id);
                        std::cout << "Result released to the student.\n";
                    }
                }
            } else if (choice == 5) {
                const auto audits = database.auditForDepartment(user);
                if (audits.empty()) std::cout << "No mark changes recorded.\n";
                for (const auto& audit : audits) {
                    std::cout << audit.id << " | " << audit.submission << " | by " << audit.changedBy << " | ";
                    if (audit.oldMark) std::cout << *audit.oldMark;
                    else std::cout << "(initial)";
                    std::cout << " -> " << audit.newMark << " | " << audit.reason << " | " << audit.changedAt << '\n';
                }
            } else {
                std::cout << "Choose an option from 0 to 5.\n";
            }
        } catch (const std::exception& error) {
            std::cerr << "Action failed: " << error.what() << '\n';
        }
    }
}

void runStudent(Database& database, const User& user, const FileStorage& files) {
    for (;;) {
        printHeader("Student");
        std::cout << "1. View enrolled courses and assignments\n2. Submit/resubmit work\n"
                     "3. View released marks and feedback\n4. Open an assignment brief\n"
                     "5. View/open my submissions\n0. Log out\n";
        const int choice = readInt("Choice: ");
        if (choice == 0) return;
        try {
            if (choice == 1 || choice == 2 || choice == 4) {
                printCourses(database, user);
                const auto assignments = database.assignmentsForStudent(user);
                printAssignments(assignments);
                if (choice == 2 && !assignments.empty()) {
                    const int id = readInt("Assignment ID (0 to cancel): ");
                    if (id != 0) {
                        const std::string source = readLine(
                            "Document file path (PDF/DOCX/ODT/RTF/TXT/PPTX/XLSX/CSV): ");
                        const std::string storedPath = files.storeDocument(source, UploadKind::Submission);
                        SubmissionReceipt receipt{};
                        try {
                            receipt = database.submit(user, id, storedPath);
                        } catch (const std::exception& error) {
                            try {
                                files.removeStoredFile(storedPath);
                            } catch (const std::exception& cleanupError) {
                                throw std::runtime_error(std::string(error.what()) +
                                    "; could not clean up uploaded submission: " + cleanupError.what());
                            }
                            throw;
                        }
                        std::cout << "Submission saved as ID " << receipt.id << ", version " << receipt.version
                                  << " (" << receipt.status << "). Stored document: " << receipt.filePath;
                        if (receipt.daysLate > 0) {
                            std::cout << " " << receipt.daysLate << " day(s) late; penalty "
                                      << receipt.penalty << " marks.";
                        }
                        if (receipt.markWillBeZero) {
                            std::cout << " More than three days late: the final mark will be zero.";
                        }
                        std::cout << '\n';
                    }
                } else if (choice == 4 && !assignments.empty()) {
                    const int id = readInt("Assignment ID whose brief to open (0 to cancel): ");
                    if (id != 0) {
                        const AssignmentRecord& assignment = findAssignment(assignments, id);
                        if (assignment.attachmentPath.empty()) {
                            std::cout << "No document is attached to this assignment.\n";
                        } else {
                            files.openDocument(assignment.attachmentPath);
                        }
                    }
                }
            } else if (choice == 5) {
                const auto submissions = database.submissionsForStudent(user);
                printStudentSubmissions(submissions);
                if (!submissions.empty()) {
                    const int id = readInt("Submission ID whose document to open (0 to cancel): ");
                    if (id != 0) files.openDocument(findSubmission(submissions, id).filePath);
                }
            } else if (choice == 3) {
                const auto marks = database.releasedMarks(user);
                if (marks.empty()) {
                    std::cout << "No results have been released. Marks remain hidden until the HoD releases "
                                 "moderated work; check option 5 for each submission's status.\n";
                }
                for (const auto& mark : marks) {
                    std::cout << mark.courseCode << " | " << mark.assignmentTitle << " | "
                              << std::fixed << std::setprecision(2) << mark.finalMark << "/" << mark.maxMark
                              << " | feedback: " << mark.feedback << '\n';
                }
            } else {
                std::cout << "Choose an option from 0 to 5.\n";
            }
        } catch (const std::exception& error) {
            std::cerr << "Action failed: " << error.what() << '\n';
        }
    }
}

}  // namespace

int main() {
    try {
        Database database("lms.db");
        FileStorage files;
        database.initializeSchema();
        database.seedDemoAccounts();
        std::cout << "Moderated LMS - C++17 / SQLite\n"
                     "First-run demo accounts: admin/admin123, lecturer/lecturer123, "
                     "hod/hod123, student/student123\n"
                     "Change demo credentials before real use.\n";
        for (;;) {
            printHeader("Login");
            std::cout << "1. Sign in\n0. Exit\n";
            const int choice = readInt("Choice: ");
            if (choice == 0) break;
            if (choice != 1) {
                std::cout << "Choose 0 or 1.\n";
                continue;
            }
            const std::string username = readLine("Username: ");
            const std::string password = readLine("Password: ");
            auto user = database.authenticate(username, password);
            if (!user) {
                std::cout << "Invalid credentials or disabled account.\n";
                continue;
            }
            std::cout << "Welcome, " << user->username() << " (" << roleToText(user->role()) << ").\n";
            switch (user->role()) {
                case Role::Administrator: runAdmin(database, *user); break;
                case Role::Lecturer: runLecturer(database, *user, files); break;
                case Role::HeadOfDepartment: runHod(database, *user, files); break;
                case Role::Student: runStudent(database, *user, files); break;
            }
        }
    } catch (const std::exception& error) {
        if (std::string(error.what()) != "Input ended.") {
            std::cerr << "Fatal startup error: " << error.what() << '\n';
            return 1;
        }
    }
    return 0;
}
