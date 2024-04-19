#include <iostream>
#include <vector>
#include <mutex>
#include <thread>
#include <random>
#include <chrono>
#include <algorithm>
#include <deque>

// Simulation readings (defined structure).
const int READINGS_PER_MINUTE = 6; // regular intervals
const int NUM_SENSORS = 8;         // temp sensors: 8
const int SIMULATION_SECONDS = 60; // 1 hr duration (simulated: 1 min)
const int INTERVAL_SECONDS = 10;   // 10 min interval (simulated: 10 secs)

// Hold a temperature reading.
struct TemperatureReading
{
    int temperature;
    std::chrono::system_clock::time_point timestamp;
};

// Temperature sensor representation.
class TemperatureSensor
{
private:
    std::mutex mutex;                        // lock
    std::deque<TemperatureReading> readings; // storage

public:
    void addReading(int temperature, const std::chrono::system_clock::time_point &timestamp)
    {
        // lock & append temp to end of readings
        std::lock_guard<std::mutex> lock(mutex);
        readings.push_back({temperature, timestamp});
    }

    std::vector<TemperatureReading> getReadings()
    {
        // lock & retrieve all readings
        std::lock_guard<std::mutex> lock(mutex);
        return {readings.begin(), readings.end()};
    }

    void clearReadings()
    {
        // lock & clear all readings
        std::lock_guard<std::mutex> lock(mutex);
        readings.clear();
    }
};

// Temperature sensor task representation.
void sensorTask(TemperatureSensor &sensor, int seconds, const std::chrono::system_clock::time_point &start_time)
{
    // enforce randomness
    std::random_device rd;
    std::mt19937 gen(rd());
    // uniform distribution set temperature range (-100F to 70F)
    std::uniform_int_distribution<> dis(-100, 70);

    // generate readings for specified duration
    for (int i = 0; i < seconds; ++i)
    {
        int temperature = dis(gen);                               // random temp
        auto current_time = start_time + std::chrono::seconds(i); // calc time
        sensor.addReading(temperature, current_time);             // add reading

        // simulate 1 minute with 1 second
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

// Print report from sensor readings.
void compileReport(TemperatureSensor &sensor, const std::chrono::system_clock::time_point &start_time)
{
    auto readings = sensor.getReadings();

    // sort readings by temp in descending order
    std::sort(readings.begin(), readings.end(),
              [](const TemperatureReading &a, const TemperatureReading &b)
              {
                  return a.temperature > b.temperature;
              });

    // print highest
    std::cout << "Top 5 Highest Temperatures:\n";
    for (int i = 0; i < 5; ++i)
    {
        std::cout << readings[i].temperature << "F\n";
    }

    // print lowest
    std::cout << "Top 5 Lowest Temperatures:\n";
    for (size_t i = readings.size() - 5; i < readings.size(); ++i)
    {
        std::cout << readings[i].temperature << "F\n";
    }

    // calc largest temp diff
    int maxDiff = 0;
    std::chrono::system_clock::time_point startInterval, endInterval;
    for (size_t i = 0; i < readings.size(); ++i)
    {
        for (size_t j = i + 1; j < readings.size(); ++j)
        {
            auto diff = std::abs(readings[j].temperature - readings[i].temperature);
            if (diff > maxDiff)
            {
                maxDiff = diff;
                startInterval = readings[i].timestamp;
                endInterval = readings[j].timestamp;
            }
        }
    }

    // print largest temp diff & interval
    auto startSeconds = std::chrono::duration_cast<std::chrono::seconds>(startInterval - start_time).count();
    auto endSeconds = std::chrono::duration_cast<std::chrono::seconds>(endInterval - start_time).count();

    std::cout << "Largest 10-second interval temperature difference: " << maxDiff << "F\n";
    std::cout << "Time interval from " << endSeconds << " to " << startSeconds << " seconds.\n";

    // clear all readings
    sensor.clearReadings();
}

int main()
{
    auto start_time = std::chrono::system_clock::now(); // curr time

    TemperatureSensor sensor;

    // initialize 8 threads
    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_SENSORS; ++i)
    {
        threads.emplace_back(sensorTask, std::ref(sensor), SIMULATION_SECONDS, start_time);
    }

    // wait for all threads to complete
    for (auto &thread : threads)
    {
        thread.join();
    }

    // compile & print report 
    compileReport(sensor, start_time);

    return 0;
}