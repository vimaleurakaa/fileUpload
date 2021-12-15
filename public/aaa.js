const image = new Image();
image.crossOrigin = "Anonymous";
image.src = "https://files.goomego.com/apps/ro/391/images/default.jpg?1603821006";
image.addEventListener('load', () => console.log('LOADED!'));
