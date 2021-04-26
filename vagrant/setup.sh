cd ..
mkdir dass
mv * dass
cp dass/vagrant_setup/Vagrantfile .
sed -i "/add sync folder/c\config.vm.synced_folder \"$PWD/dass\", \"~/dass\"" Vagrantfile
vagrant up
vagrant ssh
