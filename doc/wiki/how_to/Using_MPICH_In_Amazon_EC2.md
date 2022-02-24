# Using MPICH In Amazon EC2

To run mpi programs on Amazon EC2, you first need to have an account to
Amazon Web Services (AWS). Assume you are familiar with AWS and you know
how to start/stop/terminate Amazon EC2 instances, you can follow the
following steps to run mpi programs within a single Amazon EC2 region or
cross regions.

## Running MPI in a Single Amazon EC2 Region

### Install MPI in your cluster

Start a MPI cluster using Amazon EC2 instances. Make sure MPICH is
properly installed in your cluster. You can download MPICH here
(http://www.mpich.org/downloads/) and follow the following steps to
install.

```
tar -xzf mpich-3.2.tar.gz
cd mpich-3.2
./configure
make
sudo make install
```
If your build was successful, you should be able to see your installed
version by typing

```
mpiexec --version`
```

### Configure your MPI cluster

You should have a key pair, `yourkey.pem`, attached to the created instances.
First setup your environmental variables as follows. You can find the
access key and secret key in your Amazon EC2 account.

```
export AWS_ACCESS_KEY_ID=[ Your access key ]
export AWS_SECRET_ACCESS_KEY=[ Your access key secret ]
```
Try ssh to other instances in your cluster to make sure that your
environmental setup is successful.

```
ssh -i path-to-the-key/yourkey.pem username@other-instance-ip
```

To enable password-less ssh between instances, do the following for each
instance in your cluster.

```
cp path-to-the-key/yourkey.pem ~/.ssh/id_rsa
```

Then you should be able to ssh by typing

```
ssh username@other-instance-ip
```

### Run MPI in your cluster

Save the ip addresses of instances in a file named `host_file`. You can
compile your mpi program and execute it as following.

```
mpicc mpi_example.c -o example
mpiexec -n 2 -f host_file ./example
```

## Running MPI across Multiple Amazon EC2 Regions

If you want to run your code across multiple cloud regions, some
modifications are required to enable network communications.

### Start your MPI cluster

Create instances in your preferred regions. Make sure the security groups
of your instances allow inbound data transfer from other cloud regions.
Install MPI as introduced above to all your cluster machines. In Amazon
EC2, you can create one instance installed with all required packages
and use the image of this instance to create other machines. This will
save you some efforts.

### Configuration

Say for example you want to execute in us east and us west regions. Note
that, now you should have two keys, each for one region. To enable
password-less ssh between instances in different regions, configure the
following files.

For instances in the us east region:

```
cp path-to-the-key/us-east-key.pem ~/.ssh/id_rsa
```

Concatenate the `\~/.ssh/authorized_key` files in both us east and us
west regions and scp the concatenated file to all instances.

```
scp -i path-to-the-key/us-west-key.pem username@us-west-instance-ip:~/.ssh/authorized_key ~/.ssh/authorized_key_uw
cat ~/.ssh/authorized_key_uw >> ~/.ssh/authorized_key
scp -i path-to-the-key/us-west-key.pem ~/.ssh/authorized_key username@us-west-instance-ip:~/.ssh/
```

For instances in the us west region:

```
cp path-to-the-key/us-west-key.pem ~/.ssh/id_rsa
```

### Run MPI in your cluster

Save the \*public\* ip addresses, usually in the form 52.19.100.32 (for
example) in your hostfile. Note, do not use the public DNS, which is
usually in the form ec2-54-100-1-1.cpmpute-1.amazonaws.com. You can run
your mpi program by typing:

```
mpiexec -localhost host_node_ip -n 2 -f host_file ./example
```

The `host_node_ip` is the public ip address of your master node.
