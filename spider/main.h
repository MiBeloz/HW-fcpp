#pragma once

#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <functional>

#include "http_utils.h"
#include "http_parser.h"
#include "../settings.h"
#include "../database.h"


std::mutex mtx;
std::condition_variable cv;
std::queue<std::function<void()>> tasks;
bool exitThreadPool = false;

std::string settingsFileName = "settings.ini";
Settings& settings = Settings::init();
std::vector<std::string> appSettings;

std::vector<std::string> uniqueWords;
std::vector<Link> uniqueLinks;

std::unique_ptr<Database> database;


void threadPoolWorker();
void parseLink(const Link& link, int depth);
void wait_user();
void addSourceLinkToDatabase(const Link& link);
void addLinksToDatabase(std::set<Link>& links);
void addWordsToDatabase(const std::map<std::string, int>& words);
void addSourceLinkWordsToDatabase(const Link& link, const std::map<std::string, int> words);
void printInfo(Link link, std::map<std::string, int>& words, std::set<Link>& links);

