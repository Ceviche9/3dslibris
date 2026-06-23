#include "app/library_controller.h"

int main() {
  app_job_t job = {};
  job.type = APP_JOB_INDEX_METADATA;
  job.book = 0;
  return job.book != 0;
}
