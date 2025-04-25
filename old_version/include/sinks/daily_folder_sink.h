#pragma once

#include <spdlog/common.h>
#include <spdlog/details/file_helper.h>
#include <spdlog/details/null_mutex.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/os.h>
#include <spdlog/details/synchronous_factory.h>

#include <chrono>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <string>

namespace spdlog {
    namespace sinks {

        /*
         * Generator of daily log file names in format parent_dir/YYYY-MM-DD/base_name.ext
         */
        struct daily_foldername_calculator
        {
            // Create filename for the form parent_dir/YYYY-MM-DD
            static filename_t calc_foldername(const filename_t& parent_dir, const filename_t& base_filename, const tm& now_tm)
            {
                std::string foldername = fmt_lib::format(
                    SPDLOG_FMT_STRING(SPDLOG_FILENAME_T("{}{}{:04d}-{:02d}-{:02d}{}")), 
                    parent_dir,
                    spdlog::details::os::folder_seps_filename, 
                    now_tm.tm_year + 1900,
                    now_tm.tm_mon + 1, 
                    now_tm.tm_mday, 
                    spdlog::details::os::folder_seps_filename);

                spdlog::details::os::create_dir(foldername);
                return fmt_lib::format(SPDLOG_FMT_STRING(SPDLOG_FILENAME_T("{}{}")), foldername, base_filename);
            }
        };

        /*
         * Rotating file sink based on date.
         * If truncate != false , the created file will be truncated.
         * If max_files > 0, retain only the last max_files and delete previous.
         */
        template<typename Mutex, typename FolderNameCalc = daily_foldername_calculator>
        class daily_folder_sink final : public base_sink<Mutex>
        {
        public:
            // create daily file sink which rotates on given time
            daily_folder_sink(filename_t parent_dir, filename_t base_filename, bool truncate = false, const file_event_handlers& event_handlers = {})
                : parent_dir_(std::move(parent_dir))
                , base_filename_(std::move(base_filename))
                , truncate_(truncate)
                , file_helper_{ event_handlers }
            {
                auto now = log_clock::now();
                auto filename = FolderNameCalc::calc_foldername(parent_dir_, base_filename_, now_tm(now));
                file_helper_.open(filename);
                rotation_tp_ = next_rotation_tp_();
            }

            filename_t filename()
            {
                std::lock_guard<Mutex> lock(base_sink<Mutex>::mutex_);
                return file_helper_.filename();
            }

        protected:
            void sink_it_(const details::log_msg& msg) override
            {
                auto time = msg.time;
                bool should_rotate = time >= rotation_tp_;
                if (should_rotate)
                {
                    auto filename = FolderNameCalc::calc_foldername(parent_dir_, base_filename_, now_tm(time));
                    file_helper_.open(filename, truncate_);
                    rotation_tp_ = next_rotation_tp_();
                }
                memory_buf_t formatted;
                base_sink<Mutex>::formatter_->format(msg, formatted);
                file_helper_.write(formatted);
            }

            void flush_() override
            {
                file_helper_.flush();
            }

        private:
            log_clock::time_point next_rotation_tp_()
            {
                auto now = log_clock::now();
                tm date = now_tm(now);
                date.tm_hour = 0;
                date.tm_min = 0;
                date.tm_sec = 0;
                auto rotation_time = log_clock::from_time_t(std::mktime(&date));
                if (rotation_time > now)
                {
                    return rotation_time;
                }
                return { rotation_time + std::chrono::hours(24) };
            }

            tm now_tm(log_clock::time_point tp)
            {
                time_t tnow = log_clock::to_time_t(tp);
                return spdlog::details::os::localtime(tnow);
            }

            filename_t parent_dir_;
            filename_t base_filename_;
            log_clock::time_point rotation_tp_;
            bool truncate_;
            details::file_helper file_helper_;
        };

        using daily_folder_sink_mt = daily_folder_sink<std::mutex>;
        using daily_folder_sink_st = daily_folder_sink<details::null_mutex>;

    } // namespace sinks

    //
    // factory functions
    //
    template<typename Factory = spdlog::synchronous_factory>
    inline std::shared_ptr<logger> daily_folder_logger_mt(const std::string& logger_name, const filename_t& parent_dir, const filename_t& filename)
    {
        return Factory::template create<sinks::daily_folder_sink_mt>(logger_name, parent_dir, filename);
    }

    template<typename Factory = spdlog::synchronous_factory>
    inline std::shared_ptr<logger> daily_folder_logger_format_mt(const std::string& logger_name, const filename_t& parent_dir, const filename_t& filename)
    {
        return Factory::template create<sinks::daily_folder_sink_mt>(logger_name, parent_dir, filename);
    }

    template<typename Factory = spdlog::synchronous_factory>
    inline std::shared_ptr<logger> daily_folder_logger_st(const std::string& logger_name, const filename_t& parent_dir, const filename_t& filename)
    {
        return Factory::template create<sinks::daily_folder_sink_st>(logger_name, parent_dir, filename);
    }

    template<typename Factory = spdlog::synchronous_factory>
    inline std::shared_ptr<logger> daily_folder_logger_format_st(const std::string& logger_name, const filename_t& parent_dir, const filename_t& filename)
    {
        return Factory::template create<sinks::daily_folder_sink_st>(logger_name, parent_dir, filename);
    }
} // namespace spdlog
