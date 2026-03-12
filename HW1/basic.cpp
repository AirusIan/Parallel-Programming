#include <mpi.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

static void usage(const char *prog) {
  if (!prog) prog = "basic";
  std::fprintf(stderr, "Usage: %s n in_file out_file\n", prog);
}

int main(int argc, char **argv) {
  MPI_Init(&argc, &argv);

  int rank = 0, size = 1;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (argc != 4) {
    if (rank == 0) usage(argv[0]);
    MPI_Finalize();
    return 1;
  }

  const std::int64_t n = std::atoll(argv[1]);
  const char *in_path = argv[2];
  const char *out_path = argv[3];
  if (n < 0) {
    if (rank == 0) std::fprintf(stderr, "Invalid n: %lld\n", (long long)n);
    MPI_Finalize();
    return 1;
  }

  const std::int64_t base = n / size;
  const std::int64_t rem = n % size;
  const std::int64_t local_n = base + (rank < rem ? 1 : 0);
  const std::int64_t start = base * rank + (rank < rem ? rank : rem);

  std::vector<float> local(static_cast<size_t>(local_n));

  MPI_File in_fh;
  MPI_File_open(MPI_COMM_WORLD, const_cast<char *>(in_path),
                MPI_MODE_RDONLY, MPI_INFO_NULL, &in_fh);

  const MPI_Offset byte_offset = static_cast<MPI_Offset>(start) * sizeof(float);
  MPI_Status status;
  MPI_File_read_at_all(in_fh, byte_offset, local.data(),
                       static_cast<int>(local_n), MPI_FLOAT, &status);
  MPI_File_close(&in_fh);

  auto do_phase = [&](int phase) {
    int local_swapped = 0;

    for (std::int64_t i = 0; i + 1 < local_n; ++i) {
      if (((start + i) & 1) == phase && local[i] > local[i + 1]) {
        std::swap(local[i], local[i + 1]);
        local_swapped = 1;
      }
    }

    if (local_n > 0 && start > 0 && ((start - 1) & 1) == phase) {
      float send_val = local[0];
      float recv_val = send_val;
      MPI_Sendrecv(&send_val, 1, MPI_FLOAT, rank - 1, phase,
                   &recv_val, 1, MPI_FLOAT, rank - 1, phase,
                   MPI_COMM_WORLD, &status);
      if (recv_val > send_val) {
        local[0] = recv_val;
        local_swapped = 1;
      }
    }

    if (local_n > 0 && start + local_n < n &&
        ((start + local_n - 1) & 1) == phase) {
      float send_val = local[local_n - 1];
      float recv_val = send_val;
      MPI_Sendrecv(&send_val, 1, MPI_FLOAT, rank + 1, phase,
                   &recv_val, 1, MPI_FLOAT, rank + 1, phase,
                   MPI_COMM_WORLD, &status);
      if (send_val > recv_val) {
        local[local_n - 1] = recv_val;
        local_swapped = 1;
      }
    }

    int global_swapped = 0;
    MPI_Allreduce(&local_swapped, &global_swapped, 1, MPI_INT, MPI_LOR,
                  MPI_COMM_WORLD);
    return global_swapped;
  };

  while (true) {
    int any_swapped = 0;
    if (do_phase(0)) any_swapped = 1;
    if (do_phase(1)) any_swapped = 1;
    if (!any_swapped) break;
  }

  MPI_File out_fh;
  MPI_File_open(MPI_COMM_WORLD, const_cast<char *>(out_path),
                MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &out_fh);
  MPI_File_write_at_all(out_fh, byte_offset, local.data(),
                        static_cast<int>(local_n), MPI_FLOAT, &status);
  MPI_File_close(&out_fh);

  MPI_Finalize();
  return 0;
}
