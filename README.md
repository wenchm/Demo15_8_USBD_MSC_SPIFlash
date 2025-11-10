# Demo15_8_USBD_MSC_SPIFlash

# create a new repository on the command line
echo "# Demo15_8_USBD_MSC_SPIFlash" >> README.md
git init
git add README.md
git commit -m "first commit"
git branch -M master
git remote -v
git remote add origin https://github.com/wenchm/Demo15_8_USBD_MSC_SPIFlash.git

git push -u origin master

# push an existing repository from the command line
git remote add origin https://github.com/wenchm/Demo15_8_USBD_MSC_SPIFlash.git
git branch -M master
git push -u origin master

# add all files and dir
git status
git add .
git commit -m "origin"
git push -u origin master