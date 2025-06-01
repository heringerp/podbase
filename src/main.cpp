#include <cstddef>
#include <iostream>
#include <fstream>
#include <sstream>
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <pugixml.hpp>
#include <string>
#include <dirent.h>
#include <args.hxx>

using namespace curlpp::options;
using namespace std;

void parseFeed(pugi::xml_document& doc) {
}

vector<string> getAllEpisodes(const string &feed) {
    vector<string> episodes;
    try {
        // Our request to be sent.
        curlpp::Easy myRequest;

        // Set the URL.
        myRequest.setOpt<curlpp::options::Url>(feed);

        // Send request and get a result.
        // By default the result goes to standard output.
        stringstream os;
        curlpp::options::WriteStream ws(&os);
        myRequest.setOpt(ws);
        myRequest.perform();
        // std::cout << os.str() << std::endl;
        string source = os.str();
        const char* csource = source.c_str();
        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_string(csource);

        if (result)
        {
            // std::cout << "XML [" << csource << "] parsed without errors, attr value: [" << doc.child("node").attribute("attr").value() << "]\n\n";
            pugi::xml_node channel = doc.child("rss").child("channel");
            // string title = channel.child_value("title");
            // string description = channel.child_value("description");
            // cout << title << ": " << description << endl;
            int counter = 0;
            int max_counter = 10;
            for (pugi::xml_node episode = channel.child("item"); episode; episode = episode.next_sibling("item")) {
                // if (max_counter <= counter++) {
                //     break;
                // }
                // string episodeTitle = episode.child_value("title");
                // string episodePubDate = episode.child_value("pubDate");
                string episodeGuid = episode.child_value("guid");
                // string episodeEnclosure = episode.child("enclosure").attribute("url").value();
                episodes.push_back(episodeGuid);
            }
        }
        else
        {
            cout << "XML [" << csource << "] parsed with errors, attr value: [" << doc.child("node").attribute("attr").value() << "]\n";
            cout << "Error description: " << result.description() << "\n";
            cout << "Error offset: " << result.offset << " (error at [..." << (csource + result.offset) << "]\n\n";
        }
    }

    catch(curlpp::RuntimeError & e)
    {
        cout << e.what() << endl;
    }

    catch(curlpp::LogicError & e)
    {
        cout << e.what() << endl;
    }
    return episodes;
}

void parseFeeds(vector<string>& feeds) {
    for (auto feed : feeds) {
        try {
            // Our request to be sent.
            curlpp::Easy myRequest;

            // Set the URL.
            myRequest.setOpt<curlpp::options::Url>(feed);

            // Send request and get a result.
            // By default the result goes to standard output.
            stringstream os;
            curlpp::options::WriteStream ws(&os);
            myRequest.setOpt(ws);
            myRequest.perform();
            // std::cout << os.str() << std::endl;
            string source = os.str();
            const char* csource = source.c_str();
            pugi::xml_document doc;
            pugi::xml_parse_result result = doc.load_string(csource);

            if (result)
            {
                // std::cout << "XML [" << csource << "] parsed without errors, attr value: [" << doc.child("node").attribute("attr").value() << "]\n\n";
                parseFeed(doc);
            }
            else
            {
                cout << "XML [" << csource << "] parsed with errors, attr value: [" << doc.child("node").attribute("attr").value() << "]\n";
                cout << "Error description: " << result.description() << "\n";
                cout << "Error offset: " << result.offset << " (error at [..." << (csource + result.offset) << "]\n\n";
            }
        }

        catch(curlpp::RuntimeError & e)
        {
            cout << e.what() << endl;
        }

        catch(curlpp::LogicError & e)
        {
            cout << e.what() << endl;
        }
    }
}

struct Feed {
    string filename;
    string name;
    vector<string> listenedEpisodes;
};

struct Episode {
    string title;
    string link;
};

bool hasEnding (std::string const &fullString, std::string const &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

string addPaths(string const &base, string const &file) {
    string fullPath = base;
    if (base.back() != '/') fullPath += '/';
    fullPath += file;
    return fullPath;
}

Feed getFeed(string feedPath) {
    ifstream feedFile(feedPath);
    string line;

    if (!feedFile.is_open()) {
        throw std::ios_base::failure("Failed to open file: " + feedPath);
    }

    string feed;
    getline(feedFile, feed);
    vector<string> listenedEpisodes;
    while (getline(feedFile, line)) {
        listenedEpisodes.push_back(line);
    }
    feedFile.close();
    Feed f{feedPath, feed, listenedEpisodes};
    return f;
}

vector<Feed> parseConfig(string configPath) {

    vector<Feed> feeds;
    DIR* dir = opendir(configPath.c_str());
    struct dirent* entry;

    if (dir) {
        while ((entry = readdir(dir)) != nullptr) {
            string name(entry->d_name);
            if (hasEnding(name, ".txt")) {
                // std::cout << entry->d_name << std::endl;
                string feedPath = addPaths(configPath, name);
                try {
                    Feed f = getFeed(feedPath);
                    feeds.push_back(f);
                } catch (const std::ios_base::failure& e) {
                    std::cerr << "File error: " << e.what() << std::endl;
                } 
            }
        }
        closedir(dir);
    }

    cout << "Read feeds" << endl;
    for (auto feed : feeds) {
        cout << "\t" << feed.name << " | " << feed.listenedEpisodes.size() << "\n";
    }
    cout << endl;
    return feeds;
}

void writeConfig(vector<Feed> feeds) {
    for (auto feed : feeds) {
        ofstream feedFile(feed.filename);

        if (!feedFile.is_open()) {
            throw std::ios_base::failure("Failed to open file: " + feed.filename);
        }

        feedFile << feed.name << "\n";
        for (auto episode : feed.listenedEpisodes) {
            feedFile << episode << "\n";
        }
        feedFile.close();
    }

    cout << "Written feeds" << endl;
    for (auto feed : feeds) {
        cout << "\t" << feed.name << " | " << feed.listenedEpisodes.size() << "\n";
    }
    cout << endl;
}

void markAllAsPlayed(vector<Feed> &feeds) {
    for (auto &feed : feeds) {
        feed.listenedEpisodes = getAllEpisodes(feed.name);
    }
}

vector<string> getDifferences(vector<string> a, vector<string> b) {
    vector<string> diff;
    sort(a.begin(), a.end());
    sort(b.begin(), b.end());
    set_difference(a.begin(), a.end(), b.begin(), b.end(), inserter(diff, diff.begin()));
    return diff;
}

void findUnplayed(vector<Feed> &feeds) {
    for (auto &feed : feeds) {
        auto allEpisodes = getAllEpisodes(feed.name);
        auto newEpisodes = getDifferences(allEpisodes, feed.listenedEpisodes);
        for (auto episode : newEpisodes) {
            cout << "new: " << episode << endl;
        }
    }
}

int main(int argc, char **argv) {
    args::ArgumentParser p("CLI podcast downloader/manager");
    args::Group commands(p, "commands");
    args::Command mark(commands, "mark", "mark all episodes as played");
    args::Command sync(commands, "sync", "download all unplayed episodes");
    args::Group arguments(p, "arguments", args::Group::Validators::DontCare, args::Options::Global);
    args::ValueFlag<std::string> gitdir(arguments, "path", "", {"git-dir"});
    args::HelpFlag h(arguments, "help", "help", {'h', "help"});
    args::PositionalList<std::string> pathsList(arguments, "paths", "files to commit");

    auto feeds = parseConfig("config");
    try {
        p.ParseCLI(argc, argv);
        if (mark) {
            std::cout << "Marking all episodes as played" << endl;
            markAllAsPlayed(feeds);
            writeConfig(feeds);
        }
        else if (sync) {
            std::cout << "sync" << endl;
            findUnplayed(feeds);
        }
        std::cout << std::endl;
    }
    catch (args::Help) {
        std::cout << p;
    }
    catch (args::Error& e) {
        std::cerr << e.what() << std::endl << p;
        return 1;
    }

    return 0;
}
