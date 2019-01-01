# -*- mode: ruby -*-
# vi: set ft=ruby :

# Vagrantfile API/syntax version. Don't touch unless you know what you're doing!
VAGRANTFILE_API_VERSION = "2"

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|
  # All Vagrant configuration is done here. The most common configuration
  # options are documented and commented below. For a complete reference,
  # please see the online documentation at vagrantup.com.

  # Every Vagrant virtual environment requires a box to build off of.

  config.vm.define 'bionic' do |c|
    c.vm.box = 'ubuntu/bionic64'
    c.vm.provider "virtualbox" do |v|
      v.memory = (ENV['VBOX_MEMORY'] || 2048).to_i
      v.cpus = 4
    end

    c.vm.provision "shell", inline: <<-EOC
apt-get update
apt-get install -y build-essential git
    EOC
  end

  config.vm.synced_folder "./", "/home/vagrant/runtc"
end
