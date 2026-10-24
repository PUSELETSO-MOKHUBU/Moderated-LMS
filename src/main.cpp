#include <iostream>
#include <string>
#include <vector>

#include "Database.h"
#include "models/User.h"
#include "models/Course.h"
#include "models/Assignment.h"
#include "models/Submission.h"
#include "models/MarkAudit.h"

struct LoginAccount {
    std::string username;
    std::string password;
    std::string displayName;
    Role role;
};

struct AssignmentRecord {
    std::string title;
    std::string deadline;
    double maxMark;
};

struct SubmissionRecord {
    std::string studentName;
    std::string assignmentTitle;
    std::string status;
    double mark;
};

void printMenuHeader(const std::string& title) {
    std::cout << "\n===== " << title << " =====\n";
}

void showMainMenu() {
    printMenuHeader("Moderated LMS");
    std::cout << "1. Login as Administrator\n";
    std::cout << "2. Login as Lecturer\n";
    std::cout << "3. Login as Head of Department\n";
    std::cout << "4. Login as Student\n";
    std::cout << "5. Exit\n";
    std::cout << "Select an option: ";
}

bool authenticate(const std::vector<LoginAccount>& accounts, const std::string& username, const std::string& password, LoginAccount& matchedAccount) {
    for (const auto& account : accounts) {
        if (account.username == username && account.password == password) {
            matchedAccount = account;
            return true;
        }
    }
    return false;
}

void showAdminDashboard() {
    printMenuHeader("Administrator Dashboard");
    std::cout << "- Create and manage users\n";
    std::cout << "- Create departments and courses\n";
    std::cout << "- Assign lecturers to courses\n";
    std::cout << "- Enrol students in courses\n";
    std::cout << "- Review system records and access control\n";
}

void showLecturerDashboard() {
    printMenuHeader("Lecturer Dashboard");
    std::cout << "- Create assignments for a course\n";
    std::cout << "- Grant individual extensions\n";
    std::cout << "- Mark student submissions\n";
    std::cout << "- Add written feedback\n";
    std::cout << "- View submissions for assigned courses\n";
}

void showHodDashboard() {
    printMenuHeader("Head of Department Dashboard");
    std::cout << "- Review all marked submissions in the department\n";
    std::cout << "- Approve or change marks\n";
    std::cout << "- Record reasons and maintain audit trail\n";
    std::cout << "- Release marks only after moderation\n";
}

void showStudentDashboard() {
    printMenuHeader("Student Dashboard");
    std::cout << "- View enrolled courses\n";
    std::cout << "- Submit or resubmit assignments\n";
    std::cout << "- Check late penalty and deadline status\n";
    std::cout << "- View released marks and feedback\n";
}

int main() {
    std::cout << "Moderated LMS - Implementation Started" << std::endl;

    Database db("lms.db");
    if (!db.initializeSchema()) {
        std::cerr << "Database initialization failed." << std::endl;
        return 1;
    }

    std::vector<LoginAccount> accounts = {
        {"admin", "admin123", "System Administrator", Role::Administrator},
        {"lecturer", "lecturer123", "Mr. Smith", Role::Lecturer},
        {"hod", "hod123", "Mrs. Mokoena", Role::HeadOfDepartment},
        {"student", "student123", "Jane Student", Role::Student}
    };

    std::vector<AssignmentRecord> assignments = {
        {"Assignment 1", "2026-11-10", 50.0},
        {"Assignment 2", "2026-11-20", 100.0}
    };

    std::vector<SubmissionRecord> submissions = {
        {"Jane Student", "Assignment 1", "Submitted", 45.0},
        {"John Student", "Assignment 2", "Marked", 67.0}
    };

    int choice = 0;
    while (choice != 5) {
        showMainMenu();
        std::cin >> choice;

        if (choice < 1 || choice > 5) {
            std::cout << "Invalid option. Please try again.\n";
            continue;
        }

        if (choice == 5) {
            std::cout << "Exiting LMS. Goodbye.\n";
            break;
        }

        std::string username;
        std::string password;
        std::cout << "Username: ";
        std::cin >> username;
        std::cout << "Password: ";
        std::cin >> password;

        LoginAccount account;
        if (!authenticate(accounts, username, password, account)) {
            std::cout << "Invalid username or password. Please try again.\n";
            continue;
        }

        std::cout << "\nWelcome, " << account.displayName << "!\n";

        switch (account.role) {
            case Role::Administrator:
                showAdminDashboard();
                std::cout << "\nExample: create department, course, user and enrolment records.\n";
                break;
            case Role::Lecturer:
                showLecturerDashboard();
                std::cout << "\nExample: create assignment titled '" << assignments[0].title << "' with deadline '" << assignments[0].deadline << "'.\n";
                break;
            case Role::HeadOfDepartment:
                showHodDashboard();
                std::cout << "\nExample: review submission '" << submissions[0].assignmentTitle << "' by '" << submissions[0].studentName << "'.\n";
                break;
            case Role::Student:
                showStudentDashboard();
                std::cout << "\nExample: submit work for '" << assignments[0].title << "' and view final mark after release.\n";
                break;
            default:
                std::cout << "Unknown role.\n";
                break;
        }
    }

    return 0;
}
