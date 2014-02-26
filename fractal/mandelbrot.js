function iterate(cx, cy, max_iter)
{
	var i;
	var x = 0.0;
	var y = 0.0;
	for(var i = 0; i < max_iter && x*x + y*y <= 4; ++i)
	{
		var tmp = 2*x*y;
		x = x*x - y*y + cx;
		y = tmp + cy;
	}
	
	return i;
}

function Mandelbrot()
{
	var data = document.getElementById('calc_data');
	var xmin = parseFloat(data.xmin.value);
	var xmax = parseFloat(data.xmax.value);
	var ymin = parseFloat(data.ymin.value);
	var ymax = parseFloat(data.ymax.value);
	var iterations = parseInt(data.iterations.value);

	var width = 800;
	var height = 600;
	var ctx = document.getElementById('mandelbrot_image').getContext("2d");
	var img = ctx.getImageData(0, 0, width, height);
	var pix = img.data;

	for(var ix = 0; ix < width; ++ix)
	{
		var x = xmin + (xmax-xmin)*ix/(width-1);
		for(var iy = 0; iy < height; ++iy)
		{
			var y = ymin + (ymax-ymin)*iy/(height-1);
			var i = iterate(x, y, iterations);
			var ppos = 4*(iy*width + ix);

			// color according to the number of iterations
			if (i == iterations) // black
			{
				pix[ppos]   = 0;
				pix[ppos+1] = 0;
				pix[ppos+2] = 0;
			}
			else
			{
				var c = 3*Math.log(i)/Math.log(iterations - 1.0);
				if (c < 1)
				{
					pix[ppos]   = 255*c;
					pix[ppos+1] = 0;
					pix[ppos+2] = 0;
				}
				else if (c < 2)
				{
					pix[ppos]   = 255;
					pix[ppos+1] = 255*(c-1);
					pix[ppos+2] = 0;
				}
				else
				{
					pix[ppos]   = 255;
					pix[ppos+1] = 255;
					pix[ppos+2] = 255*(c-2);
				}
			}
			pix[ppos+3] = 255;
		}
	}

	ctx.putImageData(img, 0/* x */, 0/* y */);
}
