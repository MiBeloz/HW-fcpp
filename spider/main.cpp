#include "main.h"


int main() {
	try {
		Link sourceLink;
		int depth = 0;

		std::cout << "Database connection established!" << std::endl;
		std::cout << "---------------------------------------" << std::endl << std::endl;

		settings.setFileName(settingsFileName);
		appSettings = std::move(settings.readSettings());

		if (std::stoi(appSettings[Spider_StartContentProtocol])) {
			sourceLink.protocol = ProtocolType::HTTPS;
		}
		else {
			sourceLink.protocol = ProtocolType::HTTP;
		}
		sourceLink.hostName = appSettings[Spider_StartContentHostName];
		sourceLink.query = appSettings[Spider_StartContentQuery];
		depth = std::stoi(appSettings[Spider_Depth]);
		database = std::make_unique<Database>(appSettings[DB_Host], appSettings[DB_Port], appSettings[DB_DBName], appSettings[DB_Username], appSettings[DB_Password]);

		int numThreads = std::thread::hardware_concurrency();
		std::vector<std::thread> threadPool;

		for (int i = 0; i < numThreads; ++i) {
			threadPool.emplace_back(threadPoolWorker);
		}

		{
			std::lock_guard<std::mutex> lock(mtx);
			tasks.push([sourceLink, depth]() { parseLink(sourceLink, depth); });
			cv.notify_one();
		}

		{
			std::lock_guard<std::mutex> lock(mtx);
			exitThreadPool = true;
			cv.notify_all();
		}

		for (auto& t : threadPool) {
			t.join();
		}
	}
	catch (const std::exception& e) {
		std::cout << e.what() << std::endl;
	}

	std::cout << "Ready!" << std::endl;
	system("pause > nul");
	return 0;
}

void threadPoolWorker() {
	std::unique_lock<std::mutex> lock(mtx);
	while (!exitThreadPool || !tasks.empty()) {
		if (tasks.empty()) {
			cv.wait(lock);
		}
		else {
			auto task = tasks.front();
			tasks.pop();
			lock.unlock();
			task();
			lock.lock();
		}
	}
}

void parseLink(const Link& link, int depth) {
	try {
		std::string html = getHtmlContent(link);
		if (html.empty()) {
			std::cout << "Failed to get HTML Content for: " + link.to_string() << std::endl << std::endl;
			return;
		}

		const HTTP_Parser parser(link, html);
		std::map<std::string, int> words = std::move(parser.get_words());
		std::set<Link> links = std::move(parser.get_links());
		
		addSourceLinkToDatabase(link);
		addWordsToDatabase(words);
		addLinksToDatabase(links);
		addSourceLinkWordsToDatabase(link, words);

		printInfo(link, words, links);

		if (depth > 0) {
			std::lock_guard<std::mutex> lock(mtx);
			for (auto& subLink : links) {
				tasks.push([subLink, depth]() { parseLink(subLink, depth - 1); });
			}
			cv.notify_one();
		}
	}
	catch (const std::exception& e) {
		std::cout << e.what() << std::endl;
	}
}

void wait_user() {
	std::cout << "\nPress any key to continue..." << std::endl;
	system("pause > nul");
	exit(0);
}

void addSourceLinkToDatabase(const Link& link) {
	try {
		database->addLink(link.to_string());
	}
	catch (...) {
		//std::cout << "\tLink \"" + link.to_string() + "\" is already in the database!" << std::endl;
	}
}

void addLinksToDatabase(std::set<Link>& links) {
	std::set<Link> validLinks;
	for (auto& el : links) {
		auto it = std::find(uniqueLinks.begin(), uniqueLinks.end(), el);
		if (it == uniqueLinks.end()) {
			validLinks.insert(el);
			uniqueLinks.push_back(el);
		}
	}
	links = std::move(validLinks);
}

void addWordsToDatabase(const std::map<std::string, int>& words) {
	for (auto& el : words) {
		auto it = std::find(uniqueWords.begin(), uniqueWords.end(), el.first);
		if (it == uniqueWords.end()) {
			try {
				database->addWord(el.first);
			}
			catch (...) {
				//std::cout << "\tWord \"" + el.first + "\" is already in the database!" << std::endl;
			}
			uniqueWords.push_back(el.first);
		}
	}
}

void addSourceLinkWordsToDatabase(const Link& link, const std::map<std::string, int> words) {
	for (const auto& el : words) {
		try {
			database->addLinkWord(link.to_string(), el.first, el.second);
		}
		catch (...) {
			try {
				database->updateLinkWord(link.to_string(), el.first, el.second);
			}
			catch (std::exception& ex) {
				std::cerr << ex.what() << std::endl;
				wait_user();
			}
		}
	}
}

void printInfo(Link link, std::map<std::string, int>& words, std::set<Link>& links) {
	std::cout << "Link: " + link.to_string() << std::endl;
	std::cout << "Found " << words.size() << " unique words for this page." << std::endl;
	std::cout << "Found " << links.size() << " unique links for this page." << std::endl;
	std::cout << "---------------------------------------" << std::endl << std::endl;
}
