#include <mpi.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

static void usage(const char *prog) {
  if (!prog) prog = "advanced";
  std::fprintf(stderr, "Usage: %s n in_file out_file\n", prog);
}

static std::int64_t local_count(std::int64_t n, int size, int rank) {
  const std::int64_t base = n / size;
  const std::int64_t rem = n % size;
  return base + (rank < rem ? 1 : 0);
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

  std::sort(local.begin(), local.end());

  auto do_phase = [&](int phase) {
    int partner = -1;
    if (phase == 0) {
      partner = (rank % 2 == 0) ? rank + 1 : rank - 1;
    } else {
      partner = (rank % 2 == 0) ? rank - 1 : rank + 1;
    }

    if (partner < 0 || partner >= size) return 0;

    const std::int64_t partner_n = local_count(n, size, partner);
    if (local_n == 0 || partner_n == 0) return 0;

    int need_merge = 0;
    float send_boundary = 0.0f;
    float recv_boundary = 0.0f;

    if (rank < partner) {
      send_boundary = local.back();
      MPI_Sendrecv(&send_boundary, 1, MPI_FLOAT, partner, 100 + phase,
                   &recv_boundary, 1, MPI_FLOAT, partner, 100 + phase,
                   MPI_COMM_WORLD, &status);
      need_merge = (send_boundary > recv_boundary);
    } else {
      send_boundary = local.front();
      MPI_Sendrecv(&send_boundary, 1, MPI_FLOAT, partner, 100 + phase,
                   &recv_boundary, 1, MPI_FLOAT, partner, 100 + phase,
                   MPI_COMM_WORLD, &status);
      need_merge = (recv_boundary > send_boundary);
    }

    if (!need_merge) return 0;

    std::vector<float> recv_buf(static_cast<size_t>(partner_n));
    MPI_Sendrecv(local.data(), static_cast<int>(local_n), MPI_FLOAT, partner,
                 200 + phase, recv_buf.data(), static_cast<int>(partner_n),
                 MPI_FLOAT, partner, 200 + phase, MPI_COMM_WORLD, &status);

    std::vector<float> merged;
    merged.reserve(static_cast<size_t>(local_n + partner_n));
    std::merge(local.begin(), local.end(), recv_buf.begin(), recv_buf.end(),
               std::back_inserter(merged));

    if (rank < partner) {
      std::copy(merged.begin(), merged.begin() + local_n, local.begin());
    } else {
      std::copy(merged.end() - local_n, merged.end(), local.begin());
    }

    return 1;
  };

  while (true) {
    int changed = 0;
    if (do_phase(0)) changed = 1;
    if (do_phase(1)) changed = 1;
    int any_changed = 0;
    MPI_Allreduce(&changed, &any_changed, 1, MPI_INT, MPI_LOR, MPI_COMM_WORLD);
    if (!any_changed) break;
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
