#include <mpe.h>
#include <mpi.h>
#include <Eigen/Dense>

#include <algorithm>    // std::min
#include <iostream>
#include <fstream>
#include <sstream>

using namespace Eigen;

class KMeans {
  private:
    int numClusters_, dimension_;
    int numTrainingPoints_;

    MatrixXd trainingSet_;
    MatrixXd means_;
    MatrixXd partialMeans_;
    VectorXi classCount_;

  public:
    KMeans(int nc, int dim) : numClusters_(nc), dimension_(dim) { initTheta(); }

    void train(MatrixXd tset, int firstPoint, int lastPoint, int numSteps = 3000) {
        numTrainingPoints_ = tset.cols();
        trainingSet_ = tset;

        int rp_eid_begin, rp_eid_end, ms_eid_begin, ms_eid_end;
        MPE_Log_get_state_eventIDs(&rp_eid_begin, &rp_eid_end);
        MPE_Log_get_state_eventIDs(&ms_eid_begin, &ms_eid_end);
        MPE_Describe_state(rp_eid_begin, rp_eid_end, "reassignPoints", "blue");
        MPE_Describe_state(ms_eid_begin, ms_eid_end, "meanSummation", "red");

        int err;
        for (int i = 0; i < numSteps; ++i) {
            MPE_Log_event(rp_eid_begin, 0, "reassigning");
            reassignPoints(firstPoint, lastPoint);
            MPE_Log_event(rp_eid_end, 0, "reassigned");

            VectorXi newCount = VectorXi::Zero(numClusters_);
            MPI_Allreduce(classCount_.data(), newCount.data(), numClusters_,
                          MPI_INT, MPI_SUM, MPI_COMM_WORLD);
            MPI_Allreduce(partialMeans_.data(), means_.data(), partialMeans_.size(),
                          MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

            MPE_Log_event(ms_eid_begin, 0, "summing");
            means_ = means_.array().rowwise() / newCount.transpose().array().cast<double>();
            MPE_Log_event(ms_eid_end, 0, "summed");
        }

        MPI_Barrier(MPI_COMM_WORLD);
    }

    void printMeans() { std::cout << means_ << std::endl; }

  private:
    void initTheta() { means_ = MatrixXd::Random(dimension_, numClusters_); }

    bool reassignPoints(int firstPoint, int lastPoint) {
        int newCluster;
        partialMeans_ = MatrixXd::Zero(dimension_, numClusters_);
        classCount_ = VectorXi::Zero(numClusters_);

        for (int i = firstPoint; i < lastPoint; ++i) {
            (means_.colwise() - trainingSet_.col(i)).colwise().squaredNorm().minCoeff(&newCluster);

            partialMeans_.col(newCluster) += trainingSet_.col(i);
            classCount_[newCluster]++;
        }
    }
};

int main(int argc, char *argv[]) {
    double component;
    std::string line;
    int numTrainingPoints, numClusters, dimension;

    // Initialize MPI stuff.
    int nprocs, myRank;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);

    // Seed the RNG.
    srand(time(NULL));

    // Open the training file.
    std::ifstream trainingData(argv[1]);

    // Read dimensionality information.
    getline(trainingData, line);
    std::istringstream iss(line, std::istringstream::in);
    iss >> numTrainingPoints >> numClusters >> dimension;

    // Parse training point data.
    MatrixXd trainingPoints(dimension, numTrainingPoints);
    for (int n = 0; n < numTrainingPoints; ++n) {
        // Read next line (one point per line).
        getline(trainingData, line);
        std::istringstream iss(line, std::istringstream::in);

        for (int d = 0; d < dimension; ++d) {
            iss >> component;
            trainingPoints(d,n) = component;
        }
    }
    trainingData.close();

    // Decide how many points each rank is responsible for.
    int pointsPerRank = numTrainingPoints / nprocs;
    int firstPoint = myRank * pointsPerRank;
    int lastPoint = std::min(firstPoint + pointsPerRank, numTrainingPoints);

    // Initialize model and train with the data read above.
    int train_eid_begin, train_eid_end, init_eid_begin, init_eid_end;
    MPE_Log_get_state_eventIDs(&train_eid_begin, &train_eid_end);
    MPE_Describe_state(train_eid_begin, train_eid_end, "training", "green");

    KMeans g(numClusters, dimension);

    MPE_Log_event(train_eid_begin, 0, "training");
    g.train(trainingPoints, firstPoint, lastPoint);
    MPE_Log_event(train_eid_end, 0, "trained");

    // Print the good stuff.
    if (myRank == 0) {
        std::cout << "means: " << std::endl; g.printMeans();
    }

    MPI_Finalize();
    return 0;
}

